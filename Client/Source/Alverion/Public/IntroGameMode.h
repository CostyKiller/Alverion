#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/UObjectGlobals.h"

#include "IntroGameMode.generated.h"

class SWidget;

/**
 * The game mode of the intro map: the studio's mark on screen while the front end map loads behind it.
 * It travels on once the front end is in memory and the mark has been up for its minimum, whichever comes last.
 * A front end still loading when the mark's time is up goes on through the loading map, which says so in words.
 */
UCLASS()
class ALVERION_API AIntroGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	/** Reports the front end map is in memory, which is when travelling to it is cheap. */
	void handleFrontEndLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result);

	/** Reports the minimum the mark is shown for has passed. */
	void handleHoldElapsed();

	/** Travels once both of the above have happened, and does nothing until then. */
	void travelWhenReady();

	/** The screen, held so it can be taken off the viewport again on the way out. */
	TSharedPtr<SWidget> _screen;

	bool _frontEndLoaded = false;
	bool _holdElapsed = false;

	/** Set the moment the travel is asked for, so a second report cannot ask again. */
	bool _travelling = false;

	/** The timer behind the minimum, cleared on the way out so a torn down mode leaves nothing armed. */
	FTimerHandle _holdTimer;
};
