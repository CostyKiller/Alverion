#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "GameServer/GameServerProtocol.h"

#include "ChatLineWidget.generated.h"

class UEditableTextBox;
class UTextBlock;

/**
 * The line to type chat on, opened by the chat key under the chat lines at the bottom left.
 * A screen of its own, so CommonUI hands its box the keys and keeps them from the world while it is open.
 * Enter sends and closes it, Escape closes it unsent, and Tab switches between saying and shouting; a line starting with ! shouts too.
 */
UCLASS()
class ALVERION_API UChatLineWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;

protected:
	virtual void NativeOnActivated() override;
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

	/** The menu's input mode with no mouse capture, so the keys go to the box and none of them reach the world. */
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;

	/** Escape and Tab are caught before the box sees them, so Escape never reaches the game menu and Tab never moves the focus. */
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent) override;

private:
	void send(const FString& typed);
	void showChannel();

	UFUNCTION()
	void handleCommitted(const FText& text, ETextCommit::Type commitMethod);

	UPROPERTY()
	TObjectPtr<UTextBlock> _channelLabel;

	UPROPERTY()
	TObjectPtr<UEditableTextBox> _input;

	/** Kept between openings, since the stack hands back the same screen each time. */
	Network::EChatChannel _channel = Network::EChatChannel::General;
};
