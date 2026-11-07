#include "UI/ToastWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/PlayerController.h"
#include "UI/UIStyle.h"

namespace
{
	constexpr float toastHoldSeconds = 2.0f;
	constexpr float toastFadeSeconds = 0.5f;
	constexpr float toastTopMargin = 40.0f;
	constexpr float toastRadius = 8.0f;
	constexpr int32 toastMessageFontSize = 18;
	constexpr int32 toastDetailFontSize = 12;
	constexpr FLinearColor toastFill = FLinearColor(0.035f, 0.03f, 0.028f, 0.9f);
}

UToastWidget* UToastWidget::show(APlayerController& player, const FText& message, const FText& detail)
{
	UToastWidget* toast = CreateWidget<UToastWidget>(&player, UToastWidget::StaticClass());
	if (toast == nullptr)
	{
		return nullptr;
	}

	// On screen first, because that is when the toast builds the text blocks it then fills.
	toast->AddToViewport(toastZOrder);
	toast->_messageText->SetText(message);
	toast->_detailText->SetText(detail);
	toast->_detailText->SetVisibility(detail.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	return toast;
}

TSharedRef<SWidget> UToastWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		UBorder* panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		panel->SetBrush(UIStyle::stateBrush(toastFill, UIStyle::outline, toastRadius));
		panel->SetPadding(FMargin(18.0f, 10.0f));

		UOverlaySlot* panelSlot = root->AddChildToOverlay(panel);
		panelSlot->SetHorizontalAlignment(HAlign_Center);
		panelSlot->SetVerticalAlignment(VAlign_Top);
		panelSlot->SetPadding(FMargin(0.0f, toastTopMargin, 0.0f, 0.0f));

		UVerticalBox* lines = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		panel->SetContent(lines);

		_messageText = UIStyle::makeLabel(*WidgetTree, FText::GetEmpty(), toastMessageFontSize, UIStyle::labelColor);
		_messageText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* messageSlot = lines->AddChildToVerticalBox(_messageText);
		messageSlot->SetHorizontalAlignment(HAlign_Center);

		_detailText = UIStyle::makeLabel(*WidgetTree, FText::GetEmpty(), toastDetailFontSize, UIStyle::mutedColor);
		_detailText->SetJustification(ETextJustify::Center);
		UVerticalBoxSlot* detailSlot = lines->AddChildToVerticalBox(_detailText);
		detailSlot->SetHorizontalAlignment(HAlign_Center);

		WidgetTree->RootWidget = root;

		// Seen, never clicked, so whatever is under it keeps its keys and clicks.
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	return Super::RebuildWidget();
}

void UToastWidget::NativeTick(const FGeometry& geometry, float deltaSeconds)
{
	Super::NativeTick(geometry, deltaSeconds);

	_elapsed += deltaSeconds;

	const float opacity = 1.0f - FMath::Clamp((_elapsed - toastHoldSeconds) / toastFadeSeconds, 0.0f, 1.0f);
	SetRenderOpacity(opacity);

	if (opacity <= 0.0f)
	{
		RemoveFromParent();
	}
}
