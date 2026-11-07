#include "Boot/LoadingGameMode.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Boot/LoadingScreen.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

namespace loadingGameMode
{
	/** The key the travel carries the destination in, as in L_Loading?target=/Game/Maps/L_Continent. */
	const TCHAR* const targetOption = TEXT("target");

	/** Where an arrival with no destination goes, which is back to the front end rather than nowhere. A client stranded on a loading screen has no way out but the task manager. */
	const TCHAR* const fallbackMapName = TEXT("L_Login");
}

void ALoadingGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* world = GetWorld();

	// Straight onto the viewport rather than through the UI subsystem's layers, for the reason the intro map gives.
	UGameViewportClient* viewport = (world != nullptr) ? world->GetGameViewport() : nullptr;
	if (viewport != nullptr)
	{
		_screen = client::makeLoadingScreen();
		viewport->AddViewportWidgetContent(_screen.ToSharedRef());
	}

	_targetPackage = UGameplayStatics::ParseOption(OptionsString, loadingGameMode::targetOption);
	if (_targetPackage.IsEmpty() || !FPackageName::DoesPackageExist(_targetPackage))
	{
		// Said in the log rather than swallowed, because arriving here without a destination is a wiring fault in whoever travelled.
		UE_LOG(LogTemp, Warning, TEXT("LoadingGameMode was given no map to load, so it returns to the front end. Options were %s."), *OptionsString);
		_travelling = true;
		UGameplayStatics::OpenLevel(this, FName(loadingGameMode::fallbackMapName));
		return;
	}

	// Asynchronously, which is the whole point of this map existing: the game thread keeps ticking, so the screen draws and the window still answers.
	LoadPackageAsync(_targetPackage, FLoadPackageAsyncDelegate::CreateUObject(this, &ALoadingGameMode::handleTargetLoaded));
}

void ALoadingGameMode::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	// Taken off by hand, since the viewport outlives this world and would otherwise keep drawing it.
	if (_screen.IsValid())
	{
		UWorld* world = GetWorld();
		UGameViewportClient* viewport = (world != nullptr) ? world->GetGameViewport() : nullptr;
		if (viewport != nullptr)
		{
			viewport->RemoveViewportWidgetContent(_screen.ToSharedRef());
		}

		_screen.Reset();
	}

	Super::EndPlay(endPlayReason);
}

void ALoadingGameMode::handleTargetLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result)
{
	// Counted as loaded either way, because a client stuck on a loading screen is worse than a slow one.
	// A failed preload then simply loads the old way inside the open below.
	if (result != EAsyncLoadingResult::Succeeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadingGameMode could not preload %s, so it loads the slow way."), *packageName.ToString());
	}

	if (_travelling)
	{
		return;
	}

	_travelling = true;

	// The short name, which is what OpenLevel takes.
	UGameplayStatics::OpenLevel(this, FName(*FPackageName::GetShortName(_targetPackage)));
}
