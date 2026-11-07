#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "GameMenuWidget.generated.h"

class UButton;
class USoundBase;

// The Escape menu, pushed onto the GameMenu layer over a world that keeps running.
// Nothing pauses, because the server owns the simulation and the character stays in it.
UCLASS()
class ALVERION_API UGameMenuWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY(EditDefaultsOnly, Category = "Game Menu")
	TObjectPtr<USoundBase> clickSound;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> resumeButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> optionsButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> logoutButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> quitButton;

private:
	void playClickSound() const;

	UFUNCTION()
	void handleResumeClicked();

	UFUNCTION()
	void handleOptionsClicked();

	UFUNCTION()
	void handleLogoutClicked();

	UFUNCTION()
	void handleQuitClicked();
};
