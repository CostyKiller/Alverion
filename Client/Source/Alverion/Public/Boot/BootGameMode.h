#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"

#include "BootGameMode.generated.h"

class SWidget;

/**
 * The game mode of the empty map the client boots into.
 * It shows black, applies the saved settings and travels to the intro map, and nothing else, since the map it runs on holds nothing to run.
 * The settings are applied here because this is the first map with a window to apply them to, so a player never sees a frame of the defaults.
 * The engine's own first map load blocks the game thread, and an empty first map costs nothing to load, which puts the wait where an ordinary widget covers it.
 */
UCLASS()
class ALVERION_API ABootGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	/** The screen, held so it can be taken off the viewport again on the way out. */
	TSharedPtr<SWidget> _screen;
};
