#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "GameServer/GameServerProtocol.h"

#include "ChatWidget.generated.h"

class UScrollBox;

/**
 * The chat lines at the bottom left of the world, always on screen and never clicked.
 * The line to type on is a screen of its own, UChatLineWidget, opened by the chat key.
 */
UCLASS()
class ALVERION_API UChatWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void addMessage(const Network::FChatMessage& message);

	UPROPERTY()
	TObjectPtr<UScrollBox> _log;
};
