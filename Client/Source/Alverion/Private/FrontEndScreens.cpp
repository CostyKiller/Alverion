#include "FrontEndScreens.h"

#include "AppearanceChoices.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "AlverionFrontEnd"

namespace
{
	const FLinearColor panelColor(0.02f, 0.025f, 0.035f, 0.85f);
	const FLinearColor chosenColor(0.25f, 0.35f, 0.55f, 1.0f);

	FSlateFontInfo font(int32 size)
	{
		return FCoreStyle::GetDefaultFontStyle("Regular", size);
	}

	TSharedRef<SWidget> panel(const TSharedRef<SWidget>& content)
	{
		return SNew(SBox)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(460.0f)
				[
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
					.BorderBackgroundColor(panelColor)
					.Padding(28.0f)
					[
						content
					]
				]
			];
	}

	TSharedRef<STextBlock> text(const FText& value, int32 size)
	{
		return SNew(STextBlock).Text(value).Font(font(size)).ColorAndOpacity(FLinearColor::White);
	}

	TSharedRef<SButton> button(const FText& label, FOnClicked onClicked)
	{
		return SNew(SButton)
			.HAlign(HAlign_Center)
			.ContentPadding(FMargin(16.0f, 6.0f))
			.OnClicked(onClicked)
			[
				SNew(STextBlock).Text(label).Font(font(13))
			];
	}

	FMargin rowGap()
	{
		return FMargin(0.0f, 6.0f);
	}
}

void SLoginScreen::Construct(const FArguments& arguments)
{
	_onLogin = arguments._OnLogin;

	ChildSlot
	[
		panel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 16.0f)
			[
				text(LOCTEXT("Title", "Alverion"), 28)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(rowGap())
			[
				SAssignNew(_host, SEditableTextBox).HintText(LOCTEXT("Host", "Login server")).Text(FText::FromString(TEXT("127.0.0.1"))).Font(font(14))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(rowGap())
			[
				SAssignNew(_account, SEditableTextBox).HintText(LOCTEXT("Account", "Account")).Font(font(14))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(rowGap())
			[
				SAssignNew(_password, SEditableTextBox).HintText(LOCTEXT("Password", "Password")).IsPassword(true).Font(font(14)).OnTextCommitted_Lambda([this](const FText& value, ETextCommit::Type commit)
				{
					if (commit == ETextCommit::OnEnter)
					{
						login();
					}
				})
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 14.0f, 0.0f, 0.0f)
			[
				button(LOCTEXT("Login", "Log in"), FOnClicked::CreateSP(this, &SLoginScreen::login))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SAssignNew(_status, STextBlock).Font(font(12)).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)).AutoWrapText(true)
			])
	];
}

FReply SLoginScreen::login()
{
	if (!_busy)
	{
		_onLogin.ExecuteIfBound(_host->GetText().ToString(), _account->GetText().ToString(), _password->GetText().ToString());
	}

	return FReply::Handled();
}

void SLoginScreen::setStatus(const FText& value)
{
	_status->SetText(value);
}

void SLoginScreen::setBusy(bool busy)
{
	_busy = busy;
}

void SCharacterScreen::Construct(const FArguments& arguments)
{
	_characters = arguments._Characters;
	_onEnter = arguments._OnEnter;
	_onDelete = arguments._OnDelete;
	_chosen = _characters.IsEmpty() ? 0 : _characters[0].objectId;

	FSimpleDelegate onCreate = arguments._OnCreate;
	FSimpleDelegate onBack = arguments._OnBack;

	ChildSlot
	[
		panel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				text(LOCTEXT("Characters", "Characters"), 22)
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SAssignNew(_list, SVerticalBox)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
				[
					button(LOCTEXT("Enter", "Enter"), FOnClicked::CreateLambda([this]()
					{
						if (_chosen != 0)
						{
							_onEnter.ExecuteIfBound(_chosen);
						}

						return FReply::Handled();
					}))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
				[
					button(LOCTEXT("Create", "Create"), FOnClicked::CreateLambda([onCreate]()
					{
						onCreate.ExecuteIfBound();
						return FReply::Handled();
					}))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
				[
					button(LOCTEXT("Delete", "Delete"), FOnClicked::CreateLambda([this]()
					{
						if (_chosen != 0)
						{
							_onDelete.ExecuteIfBound(_chosen);
						}

						return FReply::Handled();
					}))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
				[
					button(LOCTEXT("Back", "Back"), FOnClicked::CreateLambda([onBack]()
					{
						onBack.ExecuteIfBound();
						return FReply::Handled();
					}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SAssignNew(_status, STextBlock).Font(font(12)).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)).AutoWrapText(true)
			])
	];

	rebuildList();
}

void SCharacterScreen::rebuildList()
{
	_list->ClearChildren();

	if (_characters.IsEmpty())
	{
		_list->AddSlot().AutoHeight()[text(LOCTEXT("None", "No characters yet."), 14)];
		return;
	}

	for (const Network::FCharacterListEntry& character : _characters)
	{
		const uint32 objectId = character.objectId;
		FText line = FText::Format(LOCTEXT("Line", "{0}    level {1}"), FText::FromString(character.name), FText::AsNumber(character.level));
		if (character.deleteSeconds > 0)
		{
			line = FText::Format(LOCTEXT("Deleting", "{0}    (being deleted)"), FText::FromString(character.name));
		}

		_list->AddSlot().AutoHeight().Padding(0.0f, 2.0f)
		[
			SNew(SButton)
			.ButtonColorAndOpacity(objectId == _chosen ? chosenColor : FLinearColor(0.1f, 0.1f, 0.12f, 1.0f))
			.ContentPadding(FMargin(10.0f, 6.0f))
			.OnClicked(FOnClicked::CreateLambda([this, objectId]()
			{
				_chosen = objectId;
				rebuildList();
				return FReply::Handled();
			}))
			[
				SNew(STextBlock).Text(line).Font(font(14))
			]
		];
	}
}

void SCharacterScreen::setStatus(const FText& value)
{
	_status->SetText(value);
}

void SCreateScreen::Construct(const FArguments& arguments)
{
	_onCreate = arguments._OnCreate;
	FSimpleDelegate onBack = arguments._OnBack;

	ChildSlot
	[
		panel(SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 0.0f, 0.0f, 12.0f)
			[
				text(LOCTEXT("NewCharacter", "New character"), 22)
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(rowGap())
			[
				SAssignNew(_name, SEditableTextBox).HintText(LOCTEXT("Name", "Name, 2 to 16 letters and digits")).Font(font(14))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(rowGap())
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center)
				[
					text(LOCTEXT("Body", "Body"), 14)
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this]() { return _appearance.male ? FLinearColor(0.1f, 0.1f, 0.12f, 1.0f) : chosenColor; })
					.OnClicked_Lambda([this]() { _appearance.male = false; return FReply::Handled(); })
					[
						SNew(STextBlock).Text(LOCTEXT("Female", "Female")).Font(font(13))
					]
				]
				+ SHorizontalBox::Slot().AutoWidth().Padding(2.0f)
				[
					SNew(SButton)
					.ButtonColorAndOpacity_Lambda([this]() { return _appearance.male ? chosenColor : FLinearColor(0.1f, 0.1f, 0.12f, 1.0f); })
					.OnClicked_Lambda([this]() { _appearance.male = true; return FReply::Handled(); })
					[
						SNew(STextBlock).Text(LOCTEXT("Male", "Male")).Font(font(13))
					]
				]
			]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Head", "Head"), &_appearance.head, AppearanceChoices::heads)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Skin", "Skin"), &_appearance.skinTone, AppearanceChoices::skinTones)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Eyes", "Eyes"), &_appearance.eyeColor, AppearanceChoices::eyeColors)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Hair", "Hair"), &_appearance.hairStyle, AppearanceChoices::hairStyles)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("HairColor", "Hair colour"), &_appearance.hairColor, AppearanceChoices::hairColors)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Eyebrows", "Eyebrows"), &_appearance.eyebrows, AppearanceChoices::eyebrows)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Beard", "Beard"), &_appearance.beard, AppearanceChoices::beards)]
			+ SVerticalBox::Slot().AutoHeight()[makeChoiceRow(LOCTEXT("Mustache", "Mustache"), &_appearance.mustache, AppearanceChoices::mustaches)]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 16.0f, 0.0f, 0.0f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
				[
					button(LOCTEXT("CreateButton", "Create"), FOnClicked::CreateLambda([this]()
					{
						_onCreate.ExecuteIfBound(_name->GetText().ToString().TrimStartAndEnd(), _appearance);
						return FReply::Handled();
					}))
				]
				+ SHorizontalBox::Slot().FillWidth(1.0f).Padding(2.0f)
				[
					button(LOCTEXT("BackButton", "Back"), FOnClicked::CreateLambda([onBack]()
					{
						onBack.ExecuteIfBound();
						return FReply::Handled();
					}))
				]
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0.0f, 12.0f, 0.0f, 0.0f)
			[
				SAssignNew(_status, STextBlock).Font(font(12)).ColorAndOpacity(FLinearColor(0.8f, 0.8f, 0.8f)).AutoWrapText(true)
			])
	];
}

TSharedRef<SWidget> SCreateScreen::makeChoiceRow(const FText& label, uint8* value, uint8 count)
{
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Center).Padding(0.0f, 3.0f)
		[
			text(label, 14)
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			button(LOCTEXT("Previous", "<"), FOnClicked::CreateLambda([value, count]()
			{
				*value = static_cast<uint8>((*value + count - 1) % count);
				return FReply::Handled();
			}))
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10.0f, 0.0f)
		[
			SNew(SBox).WidthOverride(48.0f).HAlign(HAlign_Center)
			[
				SNew(STextBlock).Font(font(14)).ColorAndOpacity(FLinearColor::White).Text_Lambda([value, count]()
				{
					return FText::Format(LOCTEXT("ChoiceOf", "{0} / {1}"), FText::AsNumber(*value + 1), FText::AsNumber(count));
				})
			]
		]
		+ SHorizontalBox::Slot().AutoWidth()
		[
			button(LOCTEXT("Next", ">"), FOnClicked::CreateLambda([value, count]()
			{
				*value = static_cast<uint8>((*value + 1) % count);
				return FReply::Handled();
			}))
		];
}

void SCreateScreen::setStatus(const FText& value)
{
	_status->SetText(value);
}

#undef LOCTEXT_NAMESPACE
