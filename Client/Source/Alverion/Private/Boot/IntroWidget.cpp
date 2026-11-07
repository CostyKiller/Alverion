#include "Boot/IntroWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "UI/UIStyle.h"

TSharedRef<SWidget> UIntroWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		// The studio mark, and over it the fade that covers it on the way in and out.
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		// The logo picture filled the screen and hid the scene behind it, so a dark card stands in for it.
		FSlateBrush cardBrush;
		cardBrush.TintColor = FSlateColor(UIStyle::cardColor);
		UImage* card = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		card->SetBrush(cardBrush);
		UOverlaySlot* cardSlot = root->AddChildToOverlay(card);
		cardSlot->SetHorizontalAlignment(HAlign_Fill);
		cardSlot->SetVerticalAlignment(VAlign_Fill);

		UTextBlock* studio = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
		studio->SetText(FText::FromString(UIStyle::studioName));
		studio->SetJustification(ETextJustify::Center);

		// Spaced out, which is the whole look of the mark and is not something a font size gives.
		FSlateFontInfo font = UIStyle::font(UIStyle::studioFontSize);
		font.LetterSpacing = UIStyle::studioLetterSpacing;
		studio->SetFont(font);

		UOverlaySlot* studioSlot = root->AddChildToOverlay(studio);
		studioSlot->SetHorizontalAlignment(HAlign_Center);
		studioSlot->SetVerticalAlignment(VAlign_Center);

		fadeOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());

		UOverlaySlot* fadeSlot = root->AddChildToOverlay(fadeOverlay);
		fadeSlot->SetHorizontalAlignment(HAlign_Fill);
		fadeSlot->SetVerticalAlignment(VAlign_Fill);

		WidgetTree->RootWidget = root;
	}

	return Super::RebuildWidget();
}

void UIntroWidget::NativeConstruct()
{
	Super::NativeConstruct();

	_phase = IntroPhase::FadeIn;
	_elapsed = 0.0f;

	// The very first frame has to be fully opaque, or the logo flashes before the fade starts.
	applyOverlay(fadeInColor, 1.0f);

	// Keyboard focus is what makes NativeOnKeyDown fire, so the skip depends on it.
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void UIntroWidget::NativeTick(const FGeometry& geometry, float deltaSeconds)
{
	Super::NativeTick(geometry, deltaSeconds);

	_elapsed += deltaSeconds;

	switch (_phase)
	{
		case IntroPhase::FadeIn:
		{
			const float alpha = (fadeInDuration > 0.0f) ? FMath::Clamp(1.0f - (_elapsed / fadeInDuration), 0.0f, 1.0f) : 0.0f;
			applyOverlay(fadeInColor, alpha);

			if (_elapsed >= fadeInDuration)
			{
				enterPhase(IntroPhase::Hold);
			}
			break;
		}
		case IntroPhase::Hold:
		{
			if (_elapsed >= holdDuration)
			{
				enterPhase(IntroPhase::FadeOut);
			}
			break;
		}
		case IntroPhase::FadeOut:
		{
			const float alpha = (fadeOutDuration > 0.0f) ? FMath::Clamp(_elapsed / fadeOutDuration, 0.0f, 1.0f) : 1.0f;
			applyOverlay(fadeOutColor, alpha);

			if (_elapsed >= fadeOutDuration)
			{
				enterPhase(IntroPhase::Finished);
			}
			break;
		}
		default:
		{
			// Finished: nothing left to animate.
			break;
		}
	}
}

FReply UIntroWidget::NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	if (skip())
	{
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(geometry, keyEvent);
}

FReply UIntroWidget::NativeOnMouseButtonDown(const FGeometry& geometry, const FPointerEvent& pointerEvent)
{
	if (skip())
	{
		return FReply::Handled();
	}

	return Super::NativeOnMouseButtonDown(geometry, pointerEvent);
}

void UIntroWidget::enterPhase(IntroPhase phase)
{
	_phase = phase;
	_elapsed = 0.0f;

	if (phase == IntroPhase::Finished)
	{
		// Land on a fully opaque overlay so whatever follows is never seen through a partial fade.
		applyOverlay(fadeOutColor, 1.0f);
		onIntroFinished.Broadcast();
	}
}

void UIntroWidget::applyOverlay(const FLinearColor& color, float alpha)
{
	if (fadeOverlay == nullptr)
	{
		return;
	}

	FLinearColor tint = color;
	tint.A = alpha;
	fadeOverlay->SetColorAndOpacity(tint);
}

bool UIntroWidget::skip()
{
	// Skipping during the fade out would restart a dissolve that is already running.
	if ((_phase != IntroPhase::FadeIn) && (_phase != IntroPhase::Hold))
	{
		return false;
	}

	enterPhase(IntroPhase::FadeOut);
	return true;
}
