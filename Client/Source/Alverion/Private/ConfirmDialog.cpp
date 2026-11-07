#include "ConfirmDialog.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "LocalizedText.h"
#include "UIStyle.h"

namespace
{
	// Wide enough for a sentence, narrow enough to read as a dialog rather than a screen.
	constexpr float dialogWidth = 720.0f;

	// Sampled off the running dialog before the widget blueprint went away.
	// The colour lived only in that asset, so it is written down here rather than lost.
	const FLinearColor panelColor = FLinearColor::FromSRGBColor(FColor(84, 62, 47));

	constexpr float messagePadding = 18.0f;
	constexpr float buttonGap = 8.0f;

	// One button and its label, since the dialog has two that differ only in wording.
	UButton* makeDialogButton(UWidgetTree& tree, const FText& text)
	{
		UButton* button = tree.ConstructWidget<UButton>(UButton::StaticClass());
		button->SetStyle(UIStyle::buttonStyle());

		UTextBlock* label = UIStyle::makeLabel(tree, text, UIStyle::bodyFontSize, UIStyle::labelColor);
		label->SetJustification(ETextJustify::Center);

		button->AddChild(label);

		return button;
	}
}

TSharedRef<SWidget> UConfirmDialog::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		// Sinks the screen behind the question without hiding it, the same as every modal here.
		UBorder* scrim = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		scrim->SetBrushColor(UIStyle::scrimColor);
		scrim->SetHorizontalAlignment(HAlign_Center);
		scrim->SetVerticalAlignment(VAlign_Center);

		USizeBox* panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		panel->SetWidthOverride(dialogWidth);

		// Rounded to the same radius as the buttons it sits under.
		UBorder* background = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		background->SetBrush(UIStyle::roundedBrush(panelColor));
		background->SetPadding(FMargin(messagePadding));

		UVerticalBox* column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		messageText = UIStyle::makeLabel(*WidgetTree, _message, UIStyle::bodyFontSize, UIStyle::labelColor);
		messageText->SetJustification(ETextJustify::Center);

		UVerticalBoxSlot* messageSlot = column->AddChildToVerticalBox(messageText);
		messageSlot->SetHorizontalAlignment(HAlign_Center);

		UHorizontalBox* buttonRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		confirmButton = makeDialogButton(*WidgetTree, LocalizedText::get(TEXT("menu.yes")));
		cancelButton = makeDialogButton(*WidgetTree, LocalizedText::get(TEXT("menu.no")));

		buttonRow->AddChildToHorizontalBox(confirmButton);

		UHorizontalBoxSlot* cancelSlot = buttonRow->AddChildToHorizontalBox(cancelButton);
		cancelSlot->SetPadding(FMargin(buttonGap, 0.0f, 0.0f, 0.0f));

		UVerticalBoxSlot* buttonSlot = column->AddChildToVerticalBox(buttonRow);
		buttonSlot->SetHorizontalAlignment(HAlign_Center);
		buttonSlot->SetPadding(FMargin(0.0f, messagePadding, 0.0f, 0.0f));

		background->AddChild(column);
		panel->AddChild(background);
		scrim->AddChild(panel);

		WidgetTree->RootWidget = scrim;
	}

	return Super::RebuildWidget();
}

void UConfirmDialog::setup(const FText& message, FOnConfirmAnswered onAnswered)
{
	_message = message;
	_onAnswered = onAnswered;

	// Reset, because the stack hands back the same widget instance for the next question,
	// and a stale answered flag would swallow it while a stale confirmed flag would answer yes unasked.
	_confirmed = false;
	_answered = false;

	// Pushing the dialog activates it, so this can land after NativeConstruct has already run.
	if (messageText != nullptr)
	{
		messageText->SetText(_message);
	}
}

void UConfirmDialog::NativeConstruct()
{
	Super::NativeConstruct();

	// Unique, because the stack reuses this widget instance,
	// so a second question runs NativeConstruct again on buttons that are already bound,
	// and AddDynamic ensures on the duplicate.
	if (confirmButton != nullptr)
	{
		confirmButton->OnClicked.AddUniqueDynamic(this, &UConfirmDialog::handleConfirmClicked);
	}

	if (cancelButton != nullptr)
	{
		cancelButton->OnClicked.AddUniqueDynamic(this, &UConfirmDialog::handleCancelClicked);
	}

	// Set again because setup can run either side of this, depending on when the stack activates the widget.
	if (messageText != nullptr)
	{
		messageText->SetText(_message);
	}

	SetIsFocusable(true);
}

void UConfirmDialog::NativeOnDeactivated()
{
	// The single answer point. Buttons only record the choice and deactivate,
	// so escape, a gamepad back press and a pop from anywhere else all resolve the caller.
	if (!_answered)
	{
		_answered = true;
		_onAnswered.ExecuteIfBound(_confirmed);
	}

	Super::NativeOnDeactivated();
}

FReply UConfirmDialog::NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	if (keyEvent.GetKey() == EKeys::Escape)
	{
		DeactivateWidget();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(geometry, keyEvent);
}

UWidget* UConfirmDialog::NativeGetDesiredFocusTarget() const
{
	if (cancelButton != nullptr)
	{
		return cancelButton;
	}

	return Super::NativeGetDesiredFocusTarget();
}

void UConfirmDialog::handleConfirmClicked()
{
	_confirmed = true;
	DeactivateWidget();
}

void UConfirmDialog::handleCancelClicked()
{
	DeactivateWidget();
}
