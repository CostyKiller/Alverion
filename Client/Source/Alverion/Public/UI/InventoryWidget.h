#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "InventoryWidget.generated.h"

class UImage;
class UInputAction;
class UTextBlock;

/**
 * The character sheet and the bag, opened over the running world by the inventory key and closed by it or Escape.
 * A placeholder: the equipment slots and the bag are empty frames until the server's items are read.
 * The figures come from the server's latest UserInfo, and the picture is the character filmed live from the front.
 */
UCLASS()
class ALVERION_API UInventoryWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

protected:
	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** Escape, or the inventory key wherever it was rebound, closes the sheet: CommonUI holds the game's keys back while it is up. */
	virtual FReply NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;

private:
	/** Makes a label whose text is the string table entry for the key, and remembers it so refresh can follow a language change. */
	UTextBlock* makeCaption(const TCHAR* key, int32 size, const FLinearColor& colour);

	/** Sets every caption in the current language, and the name, the level line and every figure from what the server last said. */
	void refresh();

	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> _captions;

	TArray<FString> _captionKeys;

	UPROPERTY()
	TObjectPtr<UTextBlock> _nameText;

	UPROPERTY()
	TObjectPtr<UTextBlock> _summaryText;

	UPROPERTY()
	TObjectPtr<UImage> _portrait;

	/** One per stat row, in the order the rows are built. */
	UPROPERTY()
	TArray<TObjectPtr<UTextBlock>> _statValues;

	UPROPERTY()
	TObjectPtr<UInputAction> _inventoryAction;
};
