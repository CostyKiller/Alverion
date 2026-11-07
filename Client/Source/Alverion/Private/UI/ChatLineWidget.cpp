#include "UI/ChatLineWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/EditableTextBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "Input/UIActionBindingHandle.h"
#include "ServerConnectionSubsystem.h"
#include "UI/LocalizedText.h"
#include "UI/UIStyle.h"

namespace
{
	// Just under the chat lines, which sit above it at the bottom left.
	constexpr float chatLineSideMargin = 24.0f;
	constexpr float chatLineBottomMargin = 90.0f;
	constexpr float chatLineBoxWidth = 390.0f;
	constexpr int32 chatLineFontSize = 14;
	constexpr FLinearColor chatLineShoutColor = FLinearColor(1.0f, 0.62f, 0.25f, 1.0f);
	constexpr FLinearColor chatLineShadowColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.8f);
}

TSharedRef<SWidget> UChatLineWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		UHorizontalBox* row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UOverlaySlot* rowSlot = root->AddChildToOverlay(row);
		rowSlot->SetHorizontalAlignment(HAlign_Left);
		rowSlot->SetVerticalAlignment(VAlign_Bottom);
		rowSlot->SetPadding(FMargin(chatLineSideMargin, 0.0f, 0.0f, chatLineBottomMargin));

		_channelLabel = UIStyle::makeLabel(*WidgetTree, FText::GetEmpty(), chatLineFontSize, UIStyle::labelColor);
		_channelLabel->SetShadowOffset(FVector2D(1.0, 1.0));
		_channelLabel->SetShadowColorAndOpacity(chatLineShadowColor);
		UHorizontalBoxSlot* labelSlot = row->AddChildToHorizontalBox(_channelLabel);
		labelSlot->SetVerticalAlignment(VAlign_Center);
		labelSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

		USizeBox* box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		box->SetWidthOverride(chatLineBoxWidth);
		_input = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass());
		FEditableTextBoxStyle style = UIStyle::frontEndTextBoxStyle(_input->GetWidgetStyle());
		style.TextStyle.SetFont(UIStyle::font(chatLineFontSize));
		_input->SetWidgetStyle(style);
		_input->OnTextCommitted.AddUniqueDynamic(this, &UChatLineWidget::handleCommitted);
		box->AddChild(_input);
		row->AddChildToHorizontalBox(box);

		WidgetTree->RootWidget = root;

		// Only the box takes clicks; the rest of the screen lets them through.
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	return Super::RebuildWidget();
}

void UChatLineWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (_input != nullptr)
	{
		_input->SetText(FText::GetEmpty());
	}

	showChannel();
}

UWidget* UChatLineWidget::NativeGetDesiredFocusTarget() const
{
	return _input;
}

TOptional<FUIInputConfig> UChatLineWidget::GetDesiredInputConfig() const
{
	return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
}

FReply UChatLineWidget::NativeOnPreviewKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	if (keyEvent.GetKey() == EKeys::Escape)
	{
		DeactivateWidget();
		return FReply::Handled();
	}

	if (keyEvent.GetKey() == EKeys::Tab)
	{
		_channel = (_channel == Network::EChatChannel::Shout) ? Network::EChatChannel::General : Network::EChatChannel::Shout;
		showChannel();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(geometry, keyEvent);
}

void UChatLineWidget::handleCommitted(const FText& text, ETextCommit::Type commitMethod)
{
	// Only Enter closes the line from here: focus moving while the screen opens must not shut it again.
	if (commitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	send(text.ToString());
	DeactivateWidget();
}

void UChatLineWidget::send(const FString& typed)
{
	FString line = typed.TrimStartAndEnd();
	Network::EChatChannel channel = _channel;

	// A leading ! shouts, whichever channel the line is on.
	if (line.StartsWith(TEXT("!")))
	{
		channel = Network::EChatChannel::Shout;
		line.RightChopInline(1);
		line.TrimStartInline();
	}

	UGameInstance* gameInstance = GetGameInstance();
	UServerConnectionSubsystem* connection = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
	if (connection != nullptr)
	{
		connection->world().say(line, channel);
	}
}

void UChatLineWidget::showChannel()
{
	if (_channelLabel == nullptr)
	{
		return;
	}

	const bool shouting = (_channel == Network::EChatChannel::Shout);
	_channelLabel->SetText(LocalizedText::get(shouting ? TEXT("chat.shout") : TEXT("chat.say")));
	_channelLabel->SetColorAndOpacity(FSlateColor(shouting ? chatLineShoutColor : UIStyle::labelColor));
}
