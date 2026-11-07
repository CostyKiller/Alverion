#include "UI/GameMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Core/GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UI/LocalizedText.h"
#include "UI/OptionsWidget.h"
#include "Sound/SoundBase.h"
#include "UI/UIStyle.h"
#include "UI/UISubsystem.h"
#include "World/WorldGameMode.h"

namespace
{
	// Widget name to string table key, matched by name so the tree needs no extra bindings.
	const TMap<FName, FString> gameMenuTextKeys =
	{
		{ TEXT("optionsLabel"), TEXT("menu.options") },
		{ TEXT("charactersLabel"), TEXT("menu.characters") },
		{ TEXT("quitLabel"), TEXT("charselect.exit_game") }
	};

	// The dim and the button style WBP_GameMenu held, read out with the DumpWidget commandlet before the asset went away.
	constexpr FLinearColor menuBackdropColor = FLinearColor(0.01f, 0.01f, 0.015f, 0.72f);
	constexpr float menuColumnWidth = 320.0f;
	constexpr float menuButtonRadius = 14.0f;
	constexpr int32 menuFontSize = 28;

	// The close square in the top right corner, and how far the corners keep from the screen's edge.
	constexpr float closeButtonSize = 56.0f;
	constexpr float screenMargin = 32.0f;

	// One menu button. The label carries no colour on purpose, the asset left it default white.
	UButton* addMenuButton(UWidgetTree& tree, UVerticalBox& column, const TCHAR* labelName)
	{
		UButton* button = tree.ConstructWidget<UButton>(UButton::StaticClass());
		button->SetStyle(UIStyle::frontEndButtonStyle(menuButtonRadius));

		UTextBlock* label = tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(labelName));
		label->SetFont(UIStyle::font(menuFontSize));
		label->SetJustification(ETextJustify::Center);
		button->AddChild(label);

		UVerticalBoxSlot* slot = column.AddChildToVerticalBox(button);
		slot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 8.0f));

		return button;
	}
}

TSharedRef<SWidget> UGameMenuWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		// A dimmed backdrop over the running world, with the narrow button column centered on it.
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		UImage* backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush backdropBrush;
		backdropBrush.TintColor = FSlateColor(menuBackdropColor);
		backdrop->SetBrush(backdropBrush);

		UOverlaySlot* backdropSlot = root->AddChildToOverlay(backdrop);
		backdropSlot->SetHorizontalAlignment(HAlign_Fill);
		backdropSlot->SetVerticalAlignment(VAlign_Fill);

		// The close square in the top right corner.
		USizeBox* closeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		closeBox->SetWidthOverride(closeButtonSize);
		closeBox->SetHeightOverride(closeButtonSize);

		closeButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		closeButton->SetStyle(UIStyle::frontEndButtonStyle(menuButtonRadius));
		UTextBlock* closeLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(TEXT("closeLabel")));
		closeLabel->SetFont(UIStyle::font(menuFontSize));
		closeLabel->SetJustification(ETextJustify::Center);
		closeLabel->SetText(FText::FromString(TEXT("X")));
		closeButton->AddChild(closeLabel);
		closeBox->AddChild(closeButton);

		UOverlaySlot* closeSlot = root->AddChildToOverlay(closeBox);
		closeSlot->SetHorizontalAlignment(HAlign_Right);
		closeSlot->SetVerticalAlignment(VAlign_Top);
		closeSlot->SetPadding(FMargin(screenMargin));

		// Options, Characters and Exit Game stacked in the bottom right corner, where the inventory will not cover them.
		USizeBox* panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		panel->SetWidthOverride(menuColumnWidth);

		UOverlaySlot* panelSlot = root->AddChildToOverlay(panel);
		panelSlot->SetHorizontalAlignment(HAlign_Right);
		panelSlot->SetVerticalAlignment(VAlign_Bottom);
		panelSlot->SetPadding(FMargin(screenMargin));

		UVerticalBox* column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		panel->AddChild(column);

		optionsButton = addMenuButton(*WidgetTree, *column, TEXT("optionsLabel"));
		charactersButton = addMenuButton(*WidgetTree, *column, TEXT("charactersLabel"));
		quitButton = addMenuButton(*WidgetTree, *column, TEXT("quitLabel"));

		WidgetTree->RootWidget = root;

		// The class default only lived in the asset, so it is loaded here rather than lost with the blueprint.
		if (clickSound == nullptr)
		{
			clickSound = LoadObject<USoundBase>(nullptr, UIStyle::clickSoundPath);
		}
	}

	return Super::RebuildWidget();
}

void UGameMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (closeButton != nullptr)
	{
		closeButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleCloseClicked);
	}

	if (optionsButton != nullptr)
	{
		optionsButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleOptionsClicked);
	}

	if (charactersButton != nullptr)
	{
		charactersButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleCharactersClicked);
	}

	if (quitButton != nullptr)
	{
		quitButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleQuitClicked);
	}

	LocalizedText::applyToTree(WidgetTree, gameMenuTextKeys);
}

UWidget* UGameMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (optionsButton != nullptr)
	{
		return optionsButton;
	}

	return Super::NativeGetDesiredFocusTarget();
}

void UGameMenuWidget::playClickSound() const
{
	if (clickSound == nullptr)
	{
		return;
	}

	const UGameSettings* settings = UGameSettings::get();
	const float volume = (settings != nullptr) ? settings->getEffectiveSfxVolume() : 1.0f;

	UGameplayStatics::PlaySound2D(this, clickSound, volume);
}

void UGameMenuWidget::handleCloseClicked()
{
	playClickSound();

	// Deactivating pops this off its stack, which is what Escape does too.
	DeactivateWidget();
}

void UGameMenuWidget::handleOptionsClicked()
{
	playClickSound();

	UGameInstance* gameInstance = GetGameInstance();
	if (gameInstance == nullptr)
	{
		return;
	}

	UUISubsystem* subsystem = gameInstance->GetSubsystem<UUISubsystem>();
	if (subsystem == nullptr)
	{
		return;
	}

	// Options lands on the same layer, so this menu waits underneath and returns when options closes.
	// The C++ class itself, because the options screen has no widget blueprint any more.
	subsystem->pushWidget(EUILayer::GameMenu, UOptionsWidget::StaticClass());
}

void UGameMenuWidget::handleCharactersClicked()
{
	playClickSound();

	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	if (subsystem == nullptr)
	{
		return;
	}

	// The link stays up: the server takes the character out of the world and sends the list again, so no second login is needed.
	subsystem->askConfirm(LocalizedText::get(TEXT("menu.characters_confirm")), FOnConfirmAnswered::CreateWeakLambda(this, [this](bool confirmed)
	{
		UWorld* world = GetWorld();
		AWorldGameMode* mode = (world != nullptr) ? world->GetAuthGameMode<AWorldGameMode>() : nullptr;
		if (confirmed && (mode != nullptr))
		{
			DeactivateWidget();
			mode->returnToCharacterSelect();
		}
	}));
}

void UGameMenuWidget::handleQuitClicked()
{
	playClickSound();

	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	if (subsystem == nullptr)
	{
		UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		return;
	}

	// Weak, because the answer arrives after the dialog closes and this menu may be gone by then.
	subsystem->askConfirm(LocalizedText::get(TEXT("menu.exit_confirm")), FOnConfirmAnswered::CreateWeakLambda(this, [this](bool confirmed)
	{
		if (confirmed)
		{
			UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false);
		}
	}));
}
