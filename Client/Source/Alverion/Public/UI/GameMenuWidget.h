#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "GameMenuWidget.generated.h"

class UButton;
class USoundBase;

/**
 * The Escape screen in the world, pushed onto the GameMenu layer over a world that keeps running.
 * It is where equipment and the inventory will live; for now it holds Options, Characters and Exit Game, and a close button.
 * Nothing pauses, because the server owns the simulation and the character stays in it.
 */
UCLASS()
class ALVERION_API UGameMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** Escape closes the menu here: CommonUI holds the game's keys back while a menu is up, so the menu key never reaches the controller. */
	virtual FReply NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;

	UPROPERTY(EditDefaultsOnly, Category = "Game Menu")
	TObjectPtr<USoundBase> clickSound;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> closeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> optionsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> charactersButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> quitButton;

private:
	void playClickSound() const;

	UFUNCTION()
	void handleCloseClicked();

	UFUNCTION()
	void handleOptionsClicked();

	UFUNCTION()
	void handleCharactersClicked();

	UFUNCTION()
	void handleQuitClicked();
};
