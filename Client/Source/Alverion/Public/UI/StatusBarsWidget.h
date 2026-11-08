#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "StatusBarsWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * The character's HP and MP at the top left of the world screen, each a bar with its current and maximum value.
 * Read from UserInfo and kept up to date by StatusUpdate, every frame, so nothing has to tell it that a figure changed.
 */
UCLASS()
class ALVERION_API UStatusBarsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;

private:
	static constexpr int32 barCount = 2;

	void setBar(int32 index, int32 current, int32 maximum);

	UPROPERTY()
	TObjectPtr<UProgressBar> _bars[barCount];

	UPROPERTY()
	TObjectPtr<UTextBlock> _values[barCount];

	// What each bar shows now, so the text and the fill are only set again when a figure changes. -1 until the first frame.
	int32 _shownCurrent[barCount] = { -1, -1 };
	int32 _shownMaximum[barCount] = { -1, -1 };
};
