#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "ConfirmDialog.generated.h"

class UButton;
class UTextBlock;

// Answered exactly once per dialog, true only when the player chose confirm.
DECLARE_DELEGATE_OneParam(FOnConfirmAnswered, bool);

// A yes or no question on the Modal layer. The answer is sent from deactivation rather than from the buttons,
// so a gamepad back press, an escape and a confirm all end the same way,
// and no caller is ever left waiting on an answer that never arrives.
UCLASS()
class ALVERION_API UConfirmDialog : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	// Called by the subsystem right after the dialog is pushed.
	void setup(const FText& message, FOnConfirmAnswered onAnswered);

	// Builds the whole dialog. There is no widget blueprint behind this class.
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeConstruct() override;
	virtual void NativeOnDeactivated() override;
	virtual FReply NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;

protected:
	// Cancel rather than confirm, so a blind gamepad press destroys nothing.
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	UPROPERTY()
	TObjectPtr<UTextBlock> messageText;

	UPROPERTY()
	TObjectPtr<UButton> confirmButton;

	UPROPERTY()
	TObjectPtr<UButton> cancelButton;

private:
	UFUNCTION()
	void handleConfirmClicked();

	UFUNCTION()
	void handleCancelClicked();

	FOnConfirmAnswered _onAnswered;
	FText _message;
	bool _confirmed = false;
	bool _answered = false;
};
