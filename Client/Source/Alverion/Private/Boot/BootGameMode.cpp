#include "Boot/BootGameMode.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Core/GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Boot/LoadingScreen.h"

namespace bootGameMode
{
	/** The map this one exists to hand over to, which greets and loads the front end at once. Named here rather than read from a setting, since the setting names this map. */
	const TCHAR* const introMapName = TEXT("L_Intro");
}

void ABootGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Straight onto the viewport rather than through the UI subsystem's layers.
	// Those layers belong to the root layout, which belongs to the front end this is waiting for.
	UGameViewportClient* viewport = (GetWorld() != nullptr) ? GetWorld()->GetGameViewport() : nullptr;
	if (viewport != nullptr)
	{
		_screen = client::makeBootScreen();
		viewport->AddViewportWidgetContent(_screen.ToSharedRef());
	}

	// The settings, here rather than on the front end, so the first window a player sees is theirs.
	// The front end applies them too and still should, since the editor starts there and never runs this map at all.
	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->ApplySettings(false);
	}

	// The intro map is empty, so this travel costs nothing and the mark goes up immediately.
	// The front end then loads behind that mark rather than behind this black.
	UGameplayStatics::OpenLevel(this, FName(bootGameMode::introMapName));
}

void ABootGameMode::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	// Taken off by hand, since the viewport outlives this world and would otherwise keep drawing it.
	if (_screen.IsValid())
	{
		UGameViewportClient* viewport = (GetWorld() != nullptr) ? GetWorld()->GetGameViewport() : nullptr;
		if (viewport != nullptr)
		{
			viewport->RemoveViewportWidgetContent(_screen.ToSharedRef());
		}

		_screen.Reset();
	}

	Super::EndPlay(endPlayReason);
}
