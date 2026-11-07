#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"

#include "ArrivalCover.generated.h"

class UImage;

/**
 * Black over everything for a second after a map arrives, so the engine draws the new scene before anybody sees it, then a one second fade.
 * Removes itself once the screen is clear.
 */
UCLASS()
class ALVERION_API UArrivalCover : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Above every screen, the intro and the menus included. */
	static constexpr int32 coverZOrder = 1000;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;

private:
	UPROPERTY()
	TObjectPtr<UImage> _black;

	float _elapsed = 0.0f;
};
