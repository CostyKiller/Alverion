#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/UObjectGlobals.h"

#include "LoadingGameMode.generated.h"

class SWidget;

/**
 * The game mode of the loading map: the loading screen on view while another map loads in the background, then into that map.
 * The destination arrives in the travel options as target, which is how L_Loading?target=/Game/Maps/L_Continent reads.
 */
UCLASS()
class ALVERION_API ALoadingGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

private:
	void handleTargetLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result);

	TSharedPtr<SWidget> _screen;

	/** The long name of the map being waited for, empty if the travel carried none. */
	FString _targetPackage;

	bool _travelling = false;
};
