#include "GameMenuWidget.h"

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
#include "GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LocalizedText.h"
#include "OptionsWidget.h"
#include "ServerConnectionSubsystem.h"
#include "Sound/SoundBase.h"
#include "UIStyle.h"
#include "UISubsystem.h"

namespace
{
	// Widget name to string table key, matched by name so the tree needs no extra bindings.
	const TMap<FName, FString> gameMenuTextKeys =
	{
		{ TEXT("resumeLabel"), TEXT("menu.resume") },
		{ TEXT("optionsLabel"), TEXT("menu.options") },
		{ TEXT("logoutLabel"), TEXT("menu.quit_to_menu") },
		{ TEXT("quitLabel"), TEXT("menu.exit") }
	};

	// What WBP_GameMenu held, read out with the DumpWidget commandlet before the asset went away.
	constexpr FLinearColor menuBackdropColor = FLinearColor(0.01f, 0.01f, 0.015f, 0.72f);
	constexpr float menuPanelWidth = 420.0f;
	constexpr float menuButtonRadius = 14.0f;
	constexpr int32 menuFontSize = 28;

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

		USizeBox* panel = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		panel->SetWidthOverride(menuPanelWidth);

		UOverlaySlot* panelSlot = root->AddChildToOverlay(panel);
		panelSlot->SetHorizontalAlignment(HAlign_Center);
		panelSlot->SetVerticalAlignment(VAlign_Center);

		UVerticalBox* column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		panel->AddChild(column);

		resumeButton = addMenuButton(*WidgetTree, *column, TEXT("resumeLabel"));
		optionsButton = addMenuButton(*WidgetTree, *column, TEXT("optionsLabel"));
		logoutButton = addMenuButton(*WidgetTree, *column, TEXT("logoutLabel"));
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

	if (resumeButton != nullptr)
	{
		resumeButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleResumeClicked);
	}

	if (optionsButton != nullptr)
	{
		optionsButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleOptionsClicked);
	}

	if (logoutButton != nullptr)
	{
		logoutButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleLogoutClicked);
	}

	if (quitButton != nullptr)
	{
		quitButton->OnClicked.AddDynamic(this, &UGameMenuWidget::handleQuitClicked);
	}

	LocalizedText::applyToTree(WidgetTree, gameMenuTextKeys);
}

UWidget* UGameMenuWidget::NativeGetDesiredFocusTarget() const
{
	if (resumeButton != nullptr)
	{
		return resumeButton;
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

void UGameMenuWidget::handleResumeClicked()
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

void UGameMenuWidget::handleLogoutClicked()
{
	playClickSound();

	// Dropping the link is the logout: the server takes the character out of the world when its connection closes.
	UGameInstance* gameInstance = GetGameInstance();
	UServerConnectionSubsystem* network = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
	if (network != nullptr)
	{
		network->disconnectAll();
	}

	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Maps/L_Login")));
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
