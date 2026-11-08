#include "UI/StatusBarsWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "ServerConnectionSubsystem.h"
#include "UI/UIStyle.h"

namespace
{
	constexpr float barsSideMargin = 24.0f;
	constexpr float barsTopMargin = 24.0f;
	constexpr float barWidth = 260.0f;
	constexpr float barHeight = 18.0f;
	constexpr float barGap = 4.0f;
	constexpr float barCaptionWidth = 34.0f;
	constexpr int32 barFontSize = 12;
	constexpr FLinearColor barBackground = FLinearColor(0.02f, 0.018f, 0.016f, 0.85f);

	// Health, then mana, top to bottom, in the colours players of the genre expect.
	const TCHAR* const barCaptions[] = { TEXT("HP"), TEXT("MP") };
	constexpr FLinearColor barColours[] =
	{
		FLinearColor(0.72f, 0.12f, 0.1f, 1.0f),
		FLinearColor(0.15f, 0.35f, 0.82f, 1.0f)
	};
}

TSharedRef<SWidget> UStatusBarsWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		UVerticalBox* column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		UOverlaySlot* columnSlot = root->AddChildToOverlay(column);
		columnSlot->SetHorizontalAlignment(HAlign_Left);
		columnSlot->SetVerticalAlignment(VAlign_Top);
		columnSlot->SetPadding(FMargin(barsSideMargin, barsTopMargin, 0.0f, 0.0f));

		for (int32 index = 0; index < barCount; index++)
		{
			UHorizontalBox* row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

			USizeBox* captionBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			captionBox->SetWidthOverride(barCaptionWidth);
			captionBox->AddChild(UIStyle::makeLabel(*WidgetTree, FText::AsCultureInvariant(barCaptions[index]), barFontSize, UIStyle::labelColor));
			UHorizontalBoxSlot* captionSlot = row->AddChildToHorizontalBox(captionBox);
			captionSlot->SetVerticalAlignment(VAlign_Center);

			USizeBox* barBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
			barBox->SetWidthOverride(barWidth);
			barBox->SetHeightOverride(barHeight);

			UOverlay* stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

			UProgressBar* bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass());
			FProgressBarStyle style = bar->GetWidgetStyle();
			style.BackgroundImage = UIStyle::flatBrush(barBackground);
			style.FillImage = UIStyle::flatBrush(FLinearColor::White);
			bar->SetWidgetStyle(style);
			bar->SetFillColorAndOpacity(barColours[index]);
			bar->SetPercent(0.0f);
			UOverlaySlot* barSlot = stack->AddChildToOverlay(bar);
			barSlot->SetHorizontalAlignment(HAlign_Fill);
			barSlot->SetVerticalAlignment(VAlign_Fill);

			UTextBlock* value = UIStyle::makeLabel(*WidgetTree, FText::GetEmpty(), barFontSize, UIStyle::labelColor);
			UOverlaySlot* valueSlot = stack->AddChildToOverlay(value);
			valueSlot->SetHorizontalAlignment(HAlign_Center);
			valueSlot->SetVerticalAlignment(VAlign_Center);

			barBox->AddChild(stack);
			row->AddChildToHorizontalBox(barBox);

			UVerticalBoxSlot* rowSlot = column->AddChildToVerticalBox(row);
			rowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, barGap));

			_bars[index] = bar;
			_values[index] = value;
		}

		WidgetTree->RootWidget = root;

		// Seen, never clicked: the world keeps the mouse.
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	return Super::RebuildWidget();
}

void UStatusBarsWidget::NativeTick(const FGeometry& geometry, float deltaSeconds)
{
	Super::NativeTick(geometry, deltaSeconds);

	UGameInstance* gameInstance = GetGameInstance();
	UServerConnectionSubsystem* link = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
	if (link == nullptr)
	{
		return;
	}

	const Network::FUserInfo& info = link->world().userInfo();
	setBar(0, info.currentHp, info.maxHp);
	setBar(1, info.currentMp, info.maxMp);
}

void UStatusBarsWidget::setBar(int32 index, int32 current, int32 maximum)
{
	if ((_bars[index] == nullptr) || (_values[index] == nullptr))
	{
		return;
	}

	if ((current == _shownCurrent[index]) && (maximum == _shownMaximum[index]))
	{
		return;
	}

	_shownCurrent[index] = current;
	_shownMaximum[index] = maximum;

	// All zero until the first UserInfo, which shows as an empty bar rather than a division by nothing.
	const float fraction = (maximum > 0) ? FMath::Clamp(static_cast<float>(current) / static_cast<float>(maximum), 0.0f, 1.0f) : 0.0f;
	_bars[index]->SetPercent(fraction);
	_values[index]->SetText(FText::Format(INVTEXT("{0} / {1}"), FText::AsNumber(current), FText::AsNumber(maximum)));
}
