#include "Core/ClientPlayerController.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CommonActivatableWidget.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "UI/GameMenuWidget.h"
#include "UI/ChatLineWidget.h"
#include "UI/InventoryWidget.h"
#include "UI/MapWidget.h"
#include "InputAction.h"
#include "Core/InputBootstrapSubsystem.h"
#include "UI/UISubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	const TCHAR* const menuActionPath = TEXT("/Game/Input/Actions/IA_Menu.IA_Menu");
	const TCHAR* const inventoryActionPath = TEXT("/Game/Input/Actions/IA_Inventory.IA_Inventory");
	const TCHAR* const mapScreenActionPath = TEXT("/Game/Input/Actions/IA_Map.IA_Map");
	const TCHAR* const chatActionPath = TEXT("/Game/Input/Actions/IA_Chat.IA_Chat");
}

void AClientPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Enhanced Input keeps applied contexts on this controller's player input, and travel destroys the controller, so a level arrived at by travel starts with no keys until the context is applied again here.
	ULocalPlayer* localPlayer = GetLocalPlayer();
	UInputBootstrapSubsystem* bootstrap = (localPlayer != nullptr) ? localPlayer->GetSubsystem<UInputBootstrapSubsystem>() : nullptr;
	if (bootstrap != nullptr)
	{
		bootstrap->applyDefaultMappings();
	}

	UEnhancedInputComponent* enhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (enhancedInput == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientPlayerController has no Enhanced Input component, so the menu, inventory and map keys do nothing."));
		return;
	}

	UInputAction* menuAction = LoadObject<UInputAction>(nullptr, menuActionPath);
	if (menuAction != nullptr)
	{
		enhancedInput->BindAction(menuAction, ETriggerEvent::Started, this, &AClientPlayerController::toggleGameMenu);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientPlayerController could not load %s, so the menu key does nothing."), menuActionPath);
	}

	UInputAction* inventoryAction = LoadObject<UInputAction>(nullptr, inventoryActionPath);
	if (inventoryAction != nullptr)
	{
		enhancedInput->BindAction(inventoryAction, ETriggerEvent::Started, this, &AClientPlayerController::toggleInventory);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientPlayerController could not load %s, so the inventory key does nothing."), inventoryActionPath);
	}

	UInputAction* mapAction = LoadObject<UInputAction>(nullptr, mapScreenActionPath);
	if (mapAction != nullptr)
	{
		enhancedInput->BindAction(mapAction, ETriggerEvent::Started, this, &AClientPlayerController::toggleMap);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientPlayerController could not load %s, so the map key does nothing."), mapScreenActionPath);
	}

	UInputAction* chatAction = LoadObject<UInputAction>(nullptr, chatActionPath);
	if (chatAction != nullptr)
	{
		enhancedInput->BindAction(chatAction, ETriggerEvent::Started, this, &AClientPlayerController::startChat);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientPlayerController could not load %s, so the chat key does nothing."), chatActionPath);
	}

	// Alt+H for a clean screenshot: a fixed chord made here rather than a rebindable action asset, read as H with Alt held.
	if (_hideInterfaceAction == nullptr)
	{
		_hideInterfaceAction = NewObject<UInputAction>(this);
		_hideInterfaceMapping = NewObject<UInputMappingContext>(this);
		_hideInterfaceMapping->MapKey(_hideInterfaceAction, EKeys::H);
	}

	UEnhancedInputLocalPlayerSubsystem* input = (localPlayer != nullptr) ? localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (input != nullptr)
	{
		input->AddMappingContext(_hideInterfaceMapping, 1);
	}

	enhancedInput->BindAction(_hideInterfaceAction, ETriggerEvent::Started, this, &AClientPlayerController::toggleInterface);
}

void AClientPlayerController::toggleInterface()
{
	if (!IsInputKeyDown(EKeys::LeftAlt) && !IsInputKeyDown(EKeys::RightAlt))
	{
		return;
	}

	if (_interfaceHidden)
	{
		showInterface();
		return;
	}

	// Every screen on the viewport, the action bar and the layers the menus open on alike, each with the visibility it had so it comes back the same.
	TArray<UUserWidget*> screens;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, screens, UUserWidget::StaticClass(), true);
	for (UUserWidget* screen : screens)
	{
		_hiddenScreens.Emplace(screen, screen->GetVisibility());
		screen->SetVisibility(ESlateVisibility::Collapsed);
	}

	_interfaceHidden = true;
}

void AClientPlayerController::showInterface()
{
	for (const TPair<TWeakObjectPtr<UUserWidget>, ESlateVisibility>& hidden : _hiddenScreens)
	{
		if (hidden.Key.IsValid())
		{
			hidden.Key->SetVisibility(hidden.Value);
		}
	}

	_hiddenScreens.Reset();
	_interfaceHidden = false;
}

void AClientPlayerController::toggleGameMenu()
{
	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	UCommonActivatableWidgetStack* layer = (subsystem != nullptr) ? subsystem->getLayer(EUILayer::GameMenu) : nullptr;
	if ((layer == nullptr) || !subsystem->acceptEscape(false))
	{
		return;
	}

	// A second press closes what the first opened, rather than stacking another copy.
	// Options open over the menu take Escape themselves, so a press arriving here always meets the menu.
	UCommonActivatableWidget* open = layer->GetActiveWidget();
	if (open != nullptr)
	{
		open->DeactivateWidget();
		return;
	}

	openScreen(UGameMenuWidget::StaticClass());
}

void AClientPlayerController::toggleInventory()
{
	openOverBareWorld(UInventoryWidget::StaticClass());
}

void AClientPlayerController::toggleMap()
{
	openOverBareWorld(UMapWidget::StaticClass());
}

void AClientPlayerController::startChat()
{
	// A screen like the map, so CommonUI gives its box the keys; openScreen also brings back an interface Alt+H hid.
	openOverBareWorld(UChatLineWidget::StaticClass());
}

void AClientPlayerController::openOverBareWorld(TSubclassOf<UCommonActivatableWidget> screenClass)
{
	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	UCommonActivatableWidgetStack* layer = (subsystem != nullptr) ? subsystem->getLayer(EUILayer::GameMenu) : nullptr;
	if ((layer == nullptr) || (layer->GetActiveWidget() != nullptr))
	{
		return;
	}

	openScreen(screenClass);
}

void AClientPlayerController::openScreen(TSubclassOf<UCommonActivatableWidget> screenClass)
{
	// A screen opened over a hidden interface would take the keys while showing nothing.
	if (_interfaceHidden)
	{
		showInterface();
	}

	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	UCommonActivatableWidgetStack* layer = (subsystem != nullptr) ? subsystem->getLayer(EUILayer::GameMenu) : nullptr;
	if (layer == nullptr)
	{
		return;
	}

	// The layer is watched rather than the screen, because opening Options over the menu deactivates the menu without leaving it.
	if (!_watchingGameMenuLayer)
	{
		layer->OnDisplayedWidgetChanged().AddUObject(this, &AClientPlayerController::handleGameMenuLayerChanged);
		_watchingGameMenuLayer = true;
	}

	UCommonActivatableWidget* screen = subsystem->pushWidget(EUILayer::GameMenu, screenClass);
	if (screen == nullptr)
	{
		return;
	}

	// The screen is read with a pointer, and game and UI together still lets its keys through to close it again.
	SetShowMouseCursor(true);
	SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(screen->TakeWidget()));
}

void AClientPlayerController::handleGameMenuLayerChanged(UCommonActivatableWidget* widget)
{
	if (widget == nullptr)
	{
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
	}
}
