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

	/** Opens the inventory over the bare world. With a screen up CommonUI holds this key back, and the inventory closes on it by itself. */
	void toggleInventory();

	/** Pushes a screen onto the game menu layer and hands the player a cursor until the layer empties again. */
	void openScreen(TSubclassOf<UCommonActivatableWidget> screenClass);

	/** The game menu layer emptied or changed: once nothing is left on it, the world gets its keys and hidden cursor back. */
	void handleGameMenuLayerChanged(UCommonActivatableWidget* widget);

	bool _watchingGameMenuLayer = false;
};
