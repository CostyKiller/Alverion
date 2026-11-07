#include "UI/MapWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "InputAction.h"
#include "UI/LocalizedText.h"
#include "UI/UIStyle.h"
#include "UI/UISubsystem.h"

namespace
{
	const TCHAR* const mapActionPath = TEXT("/Game/Input/Actions/IA_Map.IA_Map");

	/**
	 * The continent filmed straight down, north up, east to the right, by a bake of L_Continent's terrain.
	 * The picture covers exactly this stretch of the world, in centimetres; bake again and these change with it.
	 */
	const TCHAR* const mapTexturePath = TEXT("/Game/UI/Map/T_ContinentMap.T_ContinentMap");
	constexpr double mapWorldMinX = -508010.0;
	constexpr double mapWorldMinY = 827990.0;
	constexpr double mapWorldMaxX = -255990.0;
	constexpr double mapWorldMaxY = 1056010.0;

	// The picture's size on screen before it is shrunk to fit, in the texture's own proportions of 2048 by 1853.
	constexpr float mapDesignWidth = 1000.0f;
	constexpr float mapDesignHeight = mapDesignWidth * (1853.0f / 2048.0f);

	constexpr FLinearColor mapBackdropColor = FLinearColor(0.01f, 0.01f, 0.015f, 0.55f);
	constexpr FLinearColor mapPanelFill = FLinearColor(0.035f, 0.03f, 0.028f, 0.94f);
	constexpr FLinearColor mapHeadingColor = FLinearColor(0.86f, 0.68f, 0.36f, 1.0f);
	constexpr FLinearColor mapMarkerColor = FLinearColor(1.0f, 0.78f, 0.25f, 1.0f);
	constexpr FLinearColor mapMarkerRim = FLinearColor(0.05f, 0.04f, 0.03f, 1.0f);
	constexpr float mapPanelRadius = 8.0f;
	constexpr float mapPanelPadding = 16.0f;
	constexpr int32 mapHeadingFontSize = 20;

	// The marker: a dot where the player stands with a short pointer the way they face, all turned together.
	constexpr float mapMarkerSize = 28.0f;
	constexpr float mapMarkerDot = 12.0f;
	constexpr float mapMarkerPointerLength = 9.0f;
	constexpr float mapMarkerPointerThickness = 4.0f;
}

TSharedRef<SWidget> UMapWidget::RebuildWidget()
{
	if ((WidgetTree != nullptr) && (WidgetTree->RootWidget == nullptr))
	{
		UOverlay* root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		UImage* backdrop = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		FSlateBrush backdropBrush;
		backdropBrush.TintColor = FSlateColor(mapBackdropColor);
		backdrop->SetBrush(backdropBrush);

		UOverlaySlot* backdropSlot = root->AddChildToOverlay(backdrop);
		backdropSlot->SetHorizontalAlignment(HAlign_Fill);
		backdropSlot->SetVerticalAlignment(VAlign_Fill);

		// Shrunk to fit a small window, never grown past the picture's design size.
		UScaleBox* fit = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass());
		fit->SetStretch(EStretch::ScaleToFit);
		fit->SetStretchDirection(EStretchDirection::DownOnly);

		UOverlaySlot* fitSlot = root->AddChildToOverlay(fit);
		fitSlot->SetHorizontalAlignment(HAlign_Center);
		fitSlot->SetVerticalAlignment(VAlign_Center);
		fitSlot->SetPadding(FMargin(24.0f));

		UBorder* panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		panel->SetBrush(UIStyle::stateBrush(mapPanelFill, UIStyle::outline, mapPanelRadius));
		panel->SetPadding(FMargin(mapPanelPadding));
		fit->AddChild(panel);

		UVerticalBox* column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		panel->SetContent(column);

		UTextBlock* heading = UIStyle::makeLabel(*WidgetTree, LocalizedText::get(TEXT("map.title")), mapHeadingFontSize, mapHeadingColor);
		UVerticalBoxSlot* headingSlot = column->AddChildToVerticalBox(heading);
		headingSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

		USizeBox* frame = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		frame->SetWidthOverride(mapDesignWidth);
		frame->SetHeightOverride(mapDesignHeight);
		column->AddChildToVerticalBox(frame);

		UOverlay* layers = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		frame->AddChild(layers);

		UImage* picture = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		UTexture2D* texture = LoadObject<UTexture2D>(nullptr, mapTexturePath);
		if (texture != nullptr)
		{
			picture->SetBrushFromTexture(texture, false);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("MapWidget could not load %s, so the map is blank."), mapTexturePath);
		}

		UOverlaySlot* pictureSlot = layers->AddChildToOverlay(picture);
		pictureSlot->SetHorizontalAlignment(HAlign_Fill);
		pictureSlot->SetVerticalAlignment(VAlign_Fill);

		// The marker rides a canvas the picture's own size, so a place on the map is a place on the canvas.
		UCanvasPanel* canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		UOverlaySlot* canvasSlot = layers->AddChildToOverlay(canvas);
		canvasSlot->SetHorizontalAlignment(HAlign_Fill);
		canvasSlot->SetVerticalAlignment(VAlign_Fill);

		UOverlay* marker = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

		USizeBox* dotBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		dotBox->SetWidthOverride(mapMarkerDot);
		dotBox->SetHeightOverride(mapMarkerDot);
		UBorder* dot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		dot->SetBrush(UIStyle::stateBrush(mapMarkerColor, mapMarkerRim, mapMarkerDot * 0.5f));
		dotBox->AddChild(dot);
		UOverlaySlot* dotSlot = marker->AddChildToOverlay(dotBox);
		dotSlot->SetHorizontalAlignment(HAlign_Center);
		dotSlot->SetVerticalAlignment(VAlign_Center);

		// Pointing right at no turn, which is east, the way a yaw of zero faces.
		USizeBox* pointerBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
		pointerBox->SetWidthOverride(mapMarkerPointerLength);
		pointerBox->SetHeightOverride(mapMarkerPointerThickness);
		UBorder* pointer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
		pointer->SetBrush(UIStyle::stateBrush(mapMarkerColor, mapMarkerRim, 1.0f));
		pointerBox->AddChild(pointer);
		UOverlaySlot* pointerSlot = marker->AddChildToOverlay(pointerBox);
		pointerSlot->SetHorizontalAlignment(HAlign_Right);
		pointerSlot->SetVerticalAlignment(VAlign_Center);

		_marker = marker;
		_markerSlot = canvas->AddChildToCanvas(marker);
		_markerSlot->SetSize(FVector2D(mapMarkerSize, mapMarkerSize));
		_markerSlot->SetAlignment(FVector2D(0.5, 0.5));

		WidgetTree->RootWidget = root;

		// Focusable, so the keys reach it once CommonUI puts the focus here.
		SetIsFocusable(true);
		_mapAction = LoadObject<UInputAction>(nullptr, mapActionPath);
	}

	return Super::RebuildWidget();
}

void UMapWidget::NativeOnActivated()
{
	Super::NativeOnActivated();

	placeMarker();
}

void UMapWidget::NativeTick(const FGeometry& geometry, float deltaSeconds)
{
	Super::NativeTick(geometry, deltaSeconds);

	// Auto run carries on under the map, so the marker follows.
	placeMarker();
}

void UMapWidget::placeMarker()
{
	const APawn* pawn = GetOwningPlayerPawn();
	if ((pawn == nullptr) || (_markerSlot == nullptr) || (_marker == nullptr))
	{
		return;
	}

	// East runs across the picture and south down it. A player off the mapped stretch stays pinned to its edge.
	const FVector location = pawn->GetActorLocation();
	const double across = FMath::Clamp((location.X - mapWorldMinX) / (mapWorldMaxX - mapWorldMinX), 0.0, 1.0);
	const double down = FMath::Clamp((location.Y - mapWorldMinY) / (mapWorldMaxY - mapWorldMinY), 0.0, 1.0);
	_markerSlot->SetPosition(FVector2D(across * mapDesignWidth, down * mapDesignHeight));

	// Yaw turns clockwise seen from above, and so does a widget's angle, with both at zero facing east.
	_marker->SetRenderTransformAngle(pawn->GetActorRotation().Yaw);
}

UWidget* UMapWidget::NativeGetDesiredFocusTarget() const
{
	// Nothing on the map takes input yet, so the map itself holds the focus the keys arrive through.
	return const_cast<UMapWidget*>(this);
}

FReply UMapWidget::NativeOnKeyDown(const FGeometry& geometry, const FKeyEvent& keyEvent)
{
	const FKey key = keyEvent.GetKey();
	if (key == EKeys::Escape)
	{
		UGameInstance* gameInstance = GetGameInstance();
		UUISubsystem* ui = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
		if ((ui == nullptr) || ui->acceptEscape(keyEvent.IsRepeat()))
		{
			DeactivateWidget();
		}

		return FReply::Handled();
	}

	ULocalPlayer* localPlayer = GetOwningLocalPlayer();
	const UEnhancedInputLocalPlayerSubsystem* input = (localPlayer != nullptr) ? localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;
	if (!keyEvent.IsRepeat() && (input != nullptr) && (_mapAction != nullptr) && input->QueryKeysMappedToAction(_mapAction).Contains(key))
	{
		DeactivateWidget();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(geometry, keyEvent);
}
