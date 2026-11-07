#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "MapWidget.generated.h"

class UCanvasPanelSlot;
class UInputAction;
class UWidget;

/**
 * The map of the continent, opened over the running world by the map key and closed by it or Escape.
 * The picture is the continent itself, filmed straight down and north up, and a marker shows where the player stands and which way they face.
 */
UCLASS()
class ALVERION_API UMapWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** Escape, or the map key wherever it was rebound, closes the map: CommonUI holds the game's keys back while it is up. */
	virtual FReply NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;

private:
	/** Puts the marker where the player stands and turns it the way they face. */
	void placeMarker();

	UPROPERTY()
	TObjectPtr<UWidget> _marker;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> _markerSlot;

	UPROPERTY()
	TObjectPtr<UInputAction> _mapAction;
};
