#include "Core/ClientPlayerController.h"

#include "CommonActivatableWidget.h"
#include "EnhancedInputComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "UI/GameMenuWidget.h"
#include "InputAction.h"
#include "Core/InputBootstrapSubsystem.h"
#include "UI/UISubsystem.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	const TCHAR* const menuActionPath = TEXT("/Game/Input/Actions/IA_Menu.IA_Menu");
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
	UInputAction* menuAction = LoadObject<UInputAction>(nullptr, menuActionPath);
	if ((enhancedInput == nullptr) || (menuAction == nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("ClientPlayerController could not bind %s, so the menu key does nothing."), menuActionPath);
		return;
	}

	enhancedInput->BindAction(menuAction, ETriggerEvent::Started, this, &AClientPlayerController::toggleGameMenu);
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

	// The layer is watched rather than the menu, because opening Options over the menu deactivates the menu without leaving it.
	if (!_watchingGameMenuLayer)
	{
		layer->OnDisplayedWidgetChanged().AddUObject(this, &AClientPlayerController::handleGameMenuLayerChanged);
		_watchingGameMenuLayer = true;
	}

	UCommonActivatableWidget* menu = subsystem->pushWidget(EUILayer::GameMenu, UGameMenuWidget::StaticClass());
	if (menu == nullptr)
	{
		return;
	}

	// The menu is read with a pointer, and game and UI together still lets the menu key through to close it again.
	SetShowMouseCursor(true);
	SetInputMode(FInputModeGameAndUI().SetWidgetToFocus(menu->TakeWidget()));
}

void AClientPlayerController::handleGameMenuLayerChanged(UCommonActivatableWidget* widget)
{
	if (widget == nullptr)
	{
		SetShowMouseCursor(false);
		SetInputMode(FInputModeGameOnly());
	}
}
