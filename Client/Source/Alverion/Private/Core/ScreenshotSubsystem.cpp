#include "Core/ScreenshotSubsystem.h"

#include "Engine/GameInstance.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformProperties.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UI/LocalizedText.h"
#include "UI/ToastWidget.h"
#include "UnrealClient.h"

namespace
{
	// Whether files can be made in the folder, found by making one: permissions and read-only installs show no other way.
	bool canWriteToScreenshotFolder(const FString& folder)
	{
		if (!IFileManager::Get().MakeDirectory(*folder, true))
		{
			return false;
		}

		const FString probe = folder / TEXT(".write_probe");
		if (!FFileHelper::SaveStringToFile(FString(), *probe))
		{
			return false;
		}

		IFileManager::Get().Delete(*probe);
		return true;
	}

	/** Reads F12 ahead of every widget and the game, which is how it reaches screens that keep the keys to themselves. */
	class FScreenshotKeyReader : public IInputProcessor
	{
	public:
		explicit FScreenshotKeyReader(UScreenshotSubsystem& owner)
			: _owner(&owner)
		{
		}

		virtual void Tick(const float deltaTime, FSlateApplication& slate, TSharedRef<ICursor> cursor) override
		{
		}

		virtual bool HandleKeyDownEvent(FSlateApplication& slate, const FKeyEvent& keyEvent) override
		{
			if ((keyEvent.GetKey() != EKeys::F12) || keyEvent.IsRepeat())
			{
				return false;
			}

			UScreenshotSubsystem* owner = _owner.Get();
			if (owner != nullptr)
			{
				owner->takeScreenshot();
			}

			return true;
		}

		virtual const TCHAR* GetDebugName() const override
		{
			return TEXT("ScreenshotKeyReader");
		}

	private:
		TWeakObjectPtr<UScreenshotSubsystem> _owner;
	};
}

void UScreenshotSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	// No Slate in a commandlet or a test run without a window, and nothing to press there.
	if (FSlateApplication::IsInitialized())
	{
		_keyReader = MakeShared<FScreenshotKeyReader>(*this);
		FSlateApplication::Get().RegisterInputPreProcessor(_keyReader);
	}

	FScreenshotRequest::OnScreenshotRequestProcessed().AddUObject(this, &UScreenshotSubsystem::handleScreenshotProcessed);
}

void UScreenshotSubsystem::Deinitialize()
{
	if (_keyReader.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(_keyReader);
	}

	_keyReader.Reset();
	FScreenshotRequest::OnScreenshotRequestProcessed().RemoveAll(this);

	Super::Deinitialize();
}

void UScreenshotSubsystem::takeScreenshot()
{
	// Named by the moment to the millisecond, so shots sort by time and two never share a name.
	const FString path = screenshotFolder() / FString::Printf(TEXT("Alverion_%s"), *FDateTime::Now().ToString(TEXT("%Y-%m-%d_%H-%M-%S-%s")));

	// The game view only, so a play-in-editor shot is the game rather than the whole editor window. The engine adds the .png.
	FScreenshotRequest::RequestScreenshot(path, true, false, false, FIntRect(), true);
	_pendingScreenshotName = FPaths::GetCleanFilename(path) + TEXT(".png");
	UE_LOG(LogTemp, Display, TEXT("Screenshot saved as %s.png."), *path);
}

FString UScreenshotSubsystem::screenshotFolder()
{
	if (_screenshotFolder.IsEmpty())
	{
		// Beside the executable in a packaged game, and the project folder when run from the editor, whose root is the engine's.
		const FString gameFolder = FPlatformProperties::RequiresCookedData() ? FPaths::RootDir() : FPaths::ProjectDir();
		const FString inGameFolder = FPaths::ConvertRelativePathToFull(gameFolder / TEXT("Screenshots"));

		// An install under Program Files or in a read-only tree on Linux cannot be written, so the player's Documents takes the shots instead.
		const FString inUserFolder = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPlatformProcess::UserDir(), TEXT("Alverion"), TEXT("Screenshots")));

		_screenshotFolder = canWriteToScreenshotFolder(inGameFolder) ? inGameFolder : inUserFolder;
		IFileManager::Get().MakeDirectory(*_screenshotFolder, true);
	}

	return _screenshotFolder;
}

void UScreenshotSubsystem::handleScreenshotProcessed()
{
	// Shots somebody else asked for, such as the console's, pass without a toast.
	if (_pendingScreenshotName.IsEmpty())
	{
		return;
	}

	UGameInstance* gameInstance = GetGameInstance();
	APlayerController* player = (gameInstance != nullptr) ? gameInstance->GetFirstLocalPlayerController() : nullptr;
	if (player != nullptr)
	{
		UToastWidget::show(*player, LocalizedText::get(TEXT("screenshot.saved")), FText::FromString(_pendingScreenshotName));
	}

	_pendingScreenshotName.Reset();
}
