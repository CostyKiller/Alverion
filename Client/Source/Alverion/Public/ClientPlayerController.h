#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "ClientPlayerController.generated.h"

class UCommonActivatableWidget;

/** The controller both game modes name: it puts the rebindable keys on the player again after every travel, and Menu opens or closes the game menu. */
UCLASS()
class ALVERION_API AClientPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;

private:
	void toggleGameMenu();

	/** The game menu layer emptied or changed: once nothing is left on it, the world gets its keys and hidden cursor back. */
	void handleGameMenuLayerChanged(UCommonActivatableWidget* widget);

	bool _watchingGameMenuLayer = false;
};
