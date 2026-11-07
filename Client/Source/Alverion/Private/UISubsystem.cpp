#include "UISubsystem.h"

#include "CommonActivatableWidget.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

void UUISubsystem::registerLayer(EUILayer layer, UCommonActivatableWidgetStack* stack)
{
	if (stack == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UISubsystem was asked to register layer %d with no stack."), static_cast<int32>(layer));
		return;
	}

	_layers.Add(layer, stack);
}

void UUISubsystem::unregisterLayer(EUILayer layer)
{
	_layers.Remove(layer);
}

UCommonActivatableWidget* UUISubsystem::pushWidget(EUILayer layer, TSubclassOf<UCommonActivatableWidget> widgetClass)
{
	if (widgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UISubsystem was asked to push an unset widget class onto layer %d."), static_cast<int32>(layer));
		return nullptr;
	}

	UCommonActivatableWidgetStack* stack = getLayer(layer);
	if (stack == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UISubsystem has no stack for layer %d, so %s was not pushed."), static_cast<int32>(layer), *widgetClass->GetName());
		return nullptr;
	}

	return stack->AddWidget(widgetClass);
}

void UUISubsystem::popWidget(EUILayer layer)
{
	UCommonActivatableWidgetStack* stack = getLayer(layer);
	if (stack == nullptr)
	{
		return;
	}

	UCommonActivatableWidget* active = stack->GetActiveWidget();
	if (active != nullptr)
	{
		stack->RemoveWidget(*active);
	}
}

void UUISubsystem::registerConfirmDialogClass(TSubclassOf<UConfirmDialog> dialogClass)
{
	_confirmDialogClass = dialogClass;
}

void UUISubsystem::askConfirm(const FText& message, FOnConfirmAnswered onAnswered)
{
	// Answering no beats doing the thing unasked, so every failure below takes the same exit.
	if (_confirmDialogClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UISubsystem has no confirm dialog class, so the question is answered no."));
		onAnswered.ExecuteIfBound(false);
		return;
	}

	UConfirmDialog* dialog = Cast<UConfirmDialog>(pushWidget(EUILayer::Modal, _confirmDialogClass));
	if (dialog == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("UISubsystem could not push the confirm dialog, so the question is answered no."));
		onAnswered.ExecuteIfBound(false);
		return;
	}

	// After the push, because adding the widget is what activates it and builds its tree.
	dialog->setup(message, onAnswered);
}

UCommonActivatableWidgetStack* UUISubsystem::getLayer(EUILayer layer) const
{
	const TObjectPtr<UCommonActivatableWidgetStack>* found = _layers.Find(layer);
	if (found == nullptr)
	{
		return nullptr;
	}

	return *found;
}
