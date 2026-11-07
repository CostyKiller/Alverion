#pragma once

#include "Components/SlateWrapperTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "ClientPlayerController.generated.h"

class UCommonActivatableWidget;
class UInputAction;
class UInputMappingContext;
class UUserWidget;

/** The controller both game modes name: it puts the rebindable keys on the player again after every travel, and Menu opens or closes the game menu. */
UCLASS()
class ALVERION_API AClientPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;

private:
	void toggleGameMenu();

	void toggleInventory();
	void toggleMap();

	/** Opens the inventory or the map over the bare world. With a screen up CommonUI holds these keys back, and each screen closes on its own key by itself. */
	void openOverBareWorld(TSubclassOf<UCommonActivatableWidget> screenClass);

	/** Pushes a screen onto the game menu layer and hands the player a cursor until the layer empties again. */
	void openScreen(TSubclassOf<UCommonActivatableWidget> screenClass);

	/** The game menu layer emptied or changed: once nothing is left on it, the world gets its keys and hidden cursor back. */
	void handleGameMenuLayerChanged(UCommonActivatableWidget* widget);

	/** Alt+H hides every screen on the viewport for a clean screenshot, and a second Alt+H shows them again. */
	void toggleInterface();

	/** Brings the hidden screens back as they were. */
	void showInterface();

	bool _watchingGameMenuLayer = false;

	UPROPERTY()
	TObjectPtr<UInputAction> _hideInterfaceAction;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> _hideInterfaceMapping;

	/** The screens Alt+H hid and the visibility each had, empty while everything shows. */
	TArray<TPair<TWeakObjectPtr<UUserWidget>, ESlateVisibility>> _hiddenScreens;

	bool _interfaceHidden = false;
};
