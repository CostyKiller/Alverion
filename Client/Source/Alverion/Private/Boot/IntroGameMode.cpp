#include "Boot/IntroGameMode.h"

#include "Engine/GameInstance.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Boot/LoadingScreen.h"
#include "Core/MusicSubsystem.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UObject/Package.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"

namespace introGameMode
{
	/** The map this one exists to hand over to, by package for the preload and by name for the travel. */
	const TCHAR* const frontEndMapPackage = TEXT("/Game/Maps/L_Login");
	const TCHAR* const frontEndMapName = TEXT("L_Login");

	/** The loading map, for a front end that is still loading when the mark's time is up. */
	const TCHAR* const loadingMapName = TEXT("L_Loading");

	/** The least time the mark is on screen. A greeting that comes and goes inside a frame reads as a glitch rather than a studio. */
	constexpr float minimumSeconds = 2.0f;

	/** How long the mark takes to rise out of black, and to sink back into it before the travel. */
	constexpr float fadeSeconds = 1.0f;
}

void AIntroGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Straight onto the viewport rather than through the UI subsystem's layers, which belong to the front end this is waiting for.
	UGameViewportClient* viewport = (GetWorld() != nullptr) ? GetWorld()->GetGameViewport() : nullptr;
	if (viewport != nullptr)
	{
		// The mark, and over it the black it rises out of and sinks back into.
		_shownSeconds = FPlatformTime::Seconds();
		_screen = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				client::makeIntroScreen()
			]
			+ SOverlay::Slot()
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
				.ColorAndOpacity(TAttribute<FSlateColor>::Create(TAttribute<FSlateColor>::FGetter::CreateUObject(this, &AIntroGameMode::fadeColor)))
			];
		viewport->AddViewportWidgetContent(_screen.ToSharedRef());
	}

	// The menu theme starts with the mark and plays on through whatever loads next, so a slow machine waits to music.
	UGameInstance* gameInstance = GetGameInstance();
	UMusicSubsystem* music = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UMusicSubsystem>() : nullptr;
	if (music != nullptr)
	{
		music->playMenuTheme(*this);
	}

	// Asynchronously, which is the whole point of this map existing.
	// The game thread keeps ticking, so the mark draws and the window still answers, and the travel at the end finds the packages in memory.
	LoadPackageAsync(introGameMode::frontEndMapPackage, FLoadPackageAsyncDelegate::CreateUObject(this, &AIntroGameMode::handleFrontEndLoaded));

	UWorld* world = GetWorld();
	if (world != nullptr)
	{
		world->GetTimerManager().SetTimer(_holdTimer, this, &AIntroGameMode::handleHoldElapsed, introGameMode::minimumSeconds, false);
	}
	else
	{
		// No world is no timer, and a mark that never leaves is worse than one that flashes.
		_holdElapsed = true;
	}
}

void AIntroGameMode::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UWorld* world = GetWorld();
	if (world != nullptr)
	{
		world->GetTimerManager().ClearTimer(_holdTimer);
		world->GetTimerManager().ClearTimer(_fadeTimer);
	}

	// Taken off by hand, since the viewport outlives this world and would otherwise keep drawing it.
	if (_screen.IsValid())
	{
		UGameViewportClient* viewport = (world != nullptr) ? world->GetGameViewport() : nullptr;
		if (viewport != nullptr)
		{
			viewport->RemoveViewportWidgetContent(_screen.ToSharedRef());
		}

		_screen.Reset();
	}

	Super::EndPlay(endPlayReason);
}

void AIntroGameMode::handleFrontEndLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result)
{
	// Counted as loaded either way, because a client stuck on the mark is worse than a slow one.
	// A failed preload then simply loads the old way inside the open below.
	if (result != EAsyncLoadingResult::Succeeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("IntroGameMode could not preload %s, so the front end loads the slow way."), *packageName.ToString());
	}

	_frontEndLoaded = true;
	travelWhenReady();
}

void AIntroGameMode::handleHoldElapsed()
{
	_holdElapsed = true;

	// A front end still loading once the mark has had its time is a long load, and the loading map says so in words rather than leaving the mark up.
	if (!_frontEndLoaded && !_travelling)
	{
		_travelling = true;
		leave(introGameMode::loadingMapName, FString::Printf(TEXT("target=%s"), introGameMode::frontEndMapPackage));
		return;
	}

	travelWhenReady();
}

void AIntroGameMode::travelWhenReady()
{
	// Both conditions, in either order, and once.
	if (!_frontEndLoaded || !_holdElapsed || _travelling)
	{
		return;
	}

	_travelling = true;
	leave(introGameMode::frontEndMapName, FString());
}

void AIntroGameMode::leave(const FString& mapName, const FString& options)
{
	_pendingMapName = mapName;
	_pendingOptions = options;
	_leavingSeconds = FPlatformTime::Seconds();

	UWorld* world = GetWorld();
	if (world == nullptr)
	{
		openPendingMap();
		return;
	}

	world->GetTimerManager().SetTimer(_fadeTimer, this, &AIntroGameMode::openPendingMap, introGameMode::fadeSeconds, false);
}

void AIntroGameMode::openPendingMap()
{
	UGameplayStatics::OpenLevel(this, FName(*_pendingMapName), true, _pendingOptions);
}

FSlateColor AIntroGameMode::fadeColor() const
{
	// Read by the screen every frame it draws, so the fades run on the clock rather than on ticks.
	const double now = FPlatformTime::Seconds();
	float alpha = 0.0f;
	if (_leavingSeconds > 0.0)
	{
		alpha = FMath::Clamp(static_cast<float>((now - _leavingSeconds) / introGameMode::fadeSeconds), 0.0f, 1.0f);
	}
	else
	{
		alpha = 1.0f - FMath::Clamp(static_cast<float>((now - _shownSeconds) / introGameMode::fadeSeconds), 0.0f, 1.0f);
	}

	return FSlateColor(FLinearColor(0.0f, 0.0f, 0.0f, alpha));
}
