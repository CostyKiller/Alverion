#include "UI/ActionBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "UI/UIStyle.h"

namespace
{
	constexpr int32 slotCount = 10;
	constexpr float slotSize = 52.0f;
	constexpr float slotGap = 6.0f;
	constexpr float slotRadius = 4.0f;
	constexpr float bottomMargin = 24.0f;
	constexpr int32 keyFontSize = 13;
	constexpr FLinearColor slotFill = FLinearColor(0.02f, 0.018f, 0.016f, 0.85f);
}

TSharedRef<SWidget> UActionBarWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		UHorizontalBox* row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UOverlaySlot* rowSlot = root->AddChildToOverlay(row);
		rowSlot->SetHorizontalAlignment(HAlign_Center);
		rowSlot->SetVerticalAlignment(VAlign_Bottom);
		rowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, bottomMargin));

		for (int32 index = 0; index < slotCount; index++)
		{
			USizeBox* box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			box->SetWidthOverride(slotSize);
			box->SetHeightOverride(slotSize);

			UBorder* frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
			frame->SetBrush(UIStyle::stateBrush(slotFill, UIStyle::outline, slotRadius));
			frame->SetHorizontalAlignment(HAlign_Left);
			frame->SetVerticalAlignment(VAlign_Top);
			frame->SetPadding(FMargin(6.0f, 3.0f));

			// The number keys along the top of the keyboard, 1 to 9 and then 0.
			const int32 number = (index + 1) % slotCount;
			frame->SetContent(UIStyle::makeLabel(*WidgetTree, FText::AsCultureInvariant(FString::FromInt(number)), keyFontSize, UIStyle::mutedColor));

			box->AddChild(frame);

			UHorizontalBoxSlot* slot = row->AddChildToHorizontalBox(box);
			slot->SetPadding(FMargin(slotGap * 0.5f, 0.0f));
		}

		WidgetTree->RootWidget = root;

		// Seen, never clicked: the slots hold nothing yet, and the world keeps the mouse.
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	return Super::RebuildWidget();
}
