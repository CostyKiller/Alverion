#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"

#include "ServerSelectWidget.generated.h"

class UButton;
class UServerSelectWidget;
class UVerticalBox;

// One row's click, carrying which server it was.
// UButton::OnClicked reports no sender, so the id has to be held by something bound to that row,
// and a small object per row is plainer than deducing the row from focus or hover.
UCLASS()
class UServerRowHandler : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UServerSelectWidget> owner;

	uint8 serverId = 0;

	UFUNCTION()
	void handleClicked();
};

// The login server's list of game servers, one row per server.
// Builds its own widget tree in C++ rather than binding to a blueprint,
// so the screen lives in source and needs no asset created before it can run.
UCLASS()
class ALVERION_API UServerSelectWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnActivated() override;

	// Abandons the attempt when the screen closed without a choice,
	// so a gamepad back press and the Back button end the same way.
	virtual void NativeOnDeactivated() override;

	// Called by a row. Sends the selection and closes the screen.
	void chooseServer(uint8 serverId);

protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;

private:
	// Fills the list from whatever the login server last sent.
	void populate();

	UFUNCTION()
	void handleBackClicked();

	UPROPERTY()
	TObjectPtr<UVerticalBox> _rows;

	UPROPERTY()
	TObjectPtr<UButton> _backButton;

	// The first row, which is where a gamepad should land rather than on Back.
	UPROPERTY()
	TObjectPtr<UButton> _firstRow;

	// Held so the garbage collector does not take the objects the rows are bound to.
	UPROPERTY()
	TArray<TObjectPtr<UServerRowHandler>> _handlers;

	// True once a server was chosen, so leaving without choosing can be told apart.
	bool _chose = false;
};
