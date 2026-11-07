#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "WorldEntryWidget.generated.h"

class UTextBlock;

/** Covers the character list once a character is chosen, names who is entering, and starts the trip to the world. */
UCLASS()
class ALVERION_API UWorldEntryWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;

private:
	UPROPERTY()
	TObjectPtr<UTextBlock> _characterText;

	UPROPERTY()
	TObjectPtr<UTextBlock> _statusText;
};
