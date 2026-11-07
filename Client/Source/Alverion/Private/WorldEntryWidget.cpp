#include "WorldEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "FrontEndGameMode.h"
#include "LocalizedText.h"
#include "ServerConnectionSubsystem.h"
#include "UIStyle.h"

namespace
{
	constexpr int32 characterFontSize = 26;
	constexpr float entrySpacing = 10.0f;
}

TSharedRef<SWidget> UWorldEntryWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		UBorder* backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		backdrop->SetBrushColor(UIStyle::scrimColor);
		backdrop->SetHorizontalAlignment(HAlign_Center);
		backdrop->SetVerticalAlignment(VAlign_Center);

		USizeBox* panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		panel->SetWidthOverride(UIStyle::panelWidth);

		UVerticalBox* column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		UVerticalBoxSlot* titleSlot = column->AddChildToVerticalBox(UIStyle::makeLabel(*WidgetTree, LocalizedText::get(TEXT("worldentry.title")), UIStyle::titleFontSize, UIStyle::labelColor));
		titleSlot->SetHorizontalAlignment(HAlign_Center);

		_characterText = UIStyle::makeLabel(*WidgetTree, FText::GetEmpty(), characterFontSize, UIStyle::labelColor);
		UVerticalBoxSlot* characterSlot = column->AddChildToVerticalBox(_characterText);
		characterSlot->SetHorizontalAlignment(HAlign_Center);
		characterSlot->SetPadding(FMargin(0.0f, entrySpacing, 0.0f, 0.0f));

		_statusText = UIStyle::makeLabel(*WidgetTree, FText::GetEmpty(), UIStyle::bodyFontSize, UIStyle::mutedColor);
		UVerticalBoxSlot* statusSlot = column->AddChildToVerticalBox(_statusText);
		statusSlot->SetHorizontalAlignment(HAlign_Center);
		statusSlot->SetPadding(FMargin(0.0f, entrySpacing, 0.0f, 0.0f));

		panel->AddChild(column);
		backdrop->AddChild(panel);
		WidgetTree->RootWidget = backdrop;
	}

	return Super::RebuildWidget();
}

void UWorldEntryWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	UGameInstance* gameInstance = GetGameInstance();
	UServerConnectionSubsystem* network = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
	if ((network != nullptr) && (_characterText != nullptr))
	{
		_characterText->SetText(FText::FromString(network->gameServer().selected().name));
	}

	if (_statusText != nullptr)
	{
		_statusText->SetText(FText::Format(LocalizedText::get(TEXT("worldentry.preparing")), FText::FromString(TEXT("Alverion"))));
	}

	// Nothing pops this screen. The level travel it starts is what takes the whole front end away.
	AFrontEndGameMode::travelToWorld(this);
}
