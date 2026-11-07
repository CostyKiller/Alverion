#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "IntroWidget.generated.h"

class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIntroFinished);

/**
 * Studio intro: fades in from black, holds on the studio mark, fades out to black, then broadcasts onIntroFinished.
 * Any key or mouse button cuts straight to the fade out.
 */
UCLASS()
class ALVERION_API UIntroWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Broadcast once the fade out completes, whether it ran in full or was reached by a skip.
	UPROPERTY(BlueprintAssignable, Category = "Intro")
	FIntroFinished onIntroFinished;

	// Builds the whole intro. There is no widget blueprint behind this class.
	virtual TSharedRef<SWidget> RebuildWidget() override;

	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;
	virtual FReply NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& geometry, const FPointerEvent& pointerEvent) override;

protected:
	// A one second fade in, a three second hold and a one second fade out, skippable at any point.
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	float fadeInDuration = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	float holdDuration = 3.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	float fadeOutDuration = 1.0f;

	// Black, so the studio mark rises out of the same dark it sinks back into.
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	FLinearColor fadeInColor = FLinearColor::Black;

	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	FLinearColor fadeOutColor = FLinearColor::Black;

	// Full screen image the fades are painted on, over the studio mark.
	UPROPERTY()
	TObjectPtr<UImage> fadeOverlay;

private:
	enum class IntroPhase : uint8
	{
		FadeIn,
		Hold,
		FadeOut,
		Finished
	};

	IntroPhase _phase = IntroPhase::FadeIn;
	float _elapsed = 0.0f;

	void enterPhase(IntroPhase phase);
	void applyOverlay(const FLinearColor& color, float alpha);
	bool skip();
};
