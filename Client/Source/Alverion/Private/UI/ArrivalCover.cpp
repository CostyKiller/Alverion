#include "UI/ArrivalCover.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"

namespace
{
	// Black while the arrived scene settles, then the fade.
	constexpr float coverHoldSeconds = 1.0f;
	constexpr float coverFadeSeconds = 1.0f;

	// The first frames after a load can each take far longer than this, and counted whole they would spend the hold before anything was drawn.
	constexpr float coverLongestStepSeconds = 0.1f;
}

TSharedRef<SWidget> UArrivalCover::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		_black = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		_black->SetColorAndOpacity(FLinearColor::Black);
		WidgetTree->RootWidget = _black;

		// Seen, never clicked, so the screen under it already takes its keys and clicks.
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	return Super::RebuildWidget();
}

void UArrivalCover::NativeTick(const FGeometry& geometry, float deltaSeconds)
{
	Super::NativeTick(geometry, deltaSeconds);

	_elapsed += FMath::Min(deltaSeconds, coverLongestStepSeconds);

	const float alpha = 1.0f - FMath::Clamp((_elapsed - coverHoldSeconds) / coverFadeSeconds, 0.0f, 1.0f);
	if (_black != nullptr)
	{
		_black->SetColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, alpha));
	}

	if (alpha <= 0.0f)
	{
		RemoveFromParent();
	}
}
