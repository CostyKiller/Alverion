#pragma once

#include "Appearance.h"
#include "CoreMinimal.h"
#include "GameServerPackets.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;
class SVerticalBox;

DECLARE_DELEGATE_ThreeParams(FOnLoginRequested, const FString&, const FString&, const FString&);
DECLARE_DELEGATE_OneParam(FOnCharacterChosen, uint32);
DECLARE_DELEGATE_TwoParams(FOnCreateRequested, const FString&, const Network::FAppearance&);

/** Server address, account and password. */
class SLoginScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SLoginScreen) {}
		SLATE_EVENT(FOnLoginRequested, OnLogin)
	SLATE_END_ARGS()

	void Construct(const FArguments& arguments);
	void setStatus(const FText& text);
	void setBusy(bool busy);

private:
	FReply login();

	FOnLoginRequested _onLogin;
	TSharedPtr<SEditableTextBox> _host;
	TSharedPtr<SEditableTextBox> _account;
	TSharedPtr<SEditableTextBox> _password;
	TSharedPtr<STextBlock> _status;
	bool _busy = false;
};

/** The account's characters: enter one, make one, delete one, or go back. */
class SCharacterScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCharacterScreen) {}
		SLATE_ARGUMENT(TArray<Network::FCharacterListEntry>, Characters)
		SLATE_EVENT(FOnCharacterChosen, OnEnter)
		SLATE_EVENT(FOnCharacterChosen, OnDelete)
		SLATE_EVENT(FSimpleDelegate, OnCreate)
		SLATE_EVENT(FSimpleDelegate, OnBack)
	SLATE_END_ARGS()

	void Construct(const FArguments& arguments);
	void setStatus(const FText& text);

private:
	void rebuildList();

	TArray<Network::FCharacterListEntry> _characters;
	uint32 _chosen = 0;
	FOnCharacterChosen _onEnter;
	FOnCharacterChosen _onDelete;
	TSharedPtr<SVerticalBox> _list;
	TSharedPtr<STextBlock> _status;
};

/** A name and the looks the appearance block carries. */
class SCreateScreen : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateScreen) {}
		SLATE_EVENT(FOnCreateRequested, OnCreate)
		SLATE_EVENT(FSimpleDelegate, OnBack)
	SLATE_END_ARGS()

	void Construct(const FArguments& arguments);
	void setStatus(const FText& text);

private:
	TSharedRef<SWidget> makeChoiceRow(const FText& label, uint8* value, uint8 count);

	Network::FAppearance _appearance;
	FOnCreateRequested _onCreate;
	TSharedPtr<SEditableTextBox> _name;
	TSharedPtr<STextBlock> _status;
};
