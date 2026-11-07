#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"

#include "RootLayout.generated.h"

class UCommonActivatableWidgetStack;
class UConfirmDialog;

// The one widget that stays in the viewport for the whole session. It holds the four stacks and hands them
// to the subsystem, so nothing else ever needs to know how the layers are laid out.
UCLASS()
class ALVERION_API URootLayout : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// Fades the cover that carries the first second of a map, once the map is running.
	virtual void NativeTick(const FGeometry& geometry, float deltaSeconds) override;

protected:
	// Name the four stacks like this in the widget blueprint, or the layer stays unregistered.
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> gameLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> gameMenuLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> menuLayer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonActivatableWidgetStack> modalLayer;

private:
	// Black over everything for the first second of a map, while it finishes dressing itself.
	// Above the four stacks and deaf to the mouse, so it covers the screens without taking a click.
	UPROPERTY()
	TObjectPtr<class UImage> _cover;

	// One at the first frame, zero when the cover is done and gone.
	float _coverAlpha = 1.0f;
};
