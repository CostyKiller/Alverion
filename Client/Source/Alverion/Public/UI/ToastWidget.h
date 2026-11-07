#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "ToastWidget.generated.h"

class APlayerController;
class UTextBlock;

/**
 * A short notice at the top of the screen: a message, and an optional detail line under it in smaller muted text.
 * Holds for two seconds, fades, and removes itself. Anything with a player can raise one through show.
 */
UCLASS()
class ALVERION_API UToastWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Above the world's screens, below the arrival cover. */
	static constexpr int32 toastZOrder = 900;

	/** Puts a toast on the player's screen. An empty detail leaves its line out. Callers pass translated text. */
	static UToastWidget* show(APlayerController& player, const FText& message, const FText& detail);

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> _messageText;

	UPROPERTY()
	TObjectPtr<UTextBlock> _detailText;

	float _elapsed = 0.0f;
};
