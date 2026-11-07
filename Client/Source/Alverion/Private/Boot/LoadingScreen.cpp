#include "Boot/LoadingScreen.h"

#include "Brushes/SlateDynamicImageBrush.h"
#include "Misc/Paths.h"
#include "Styling/CoreStyle.h"
#include "UI/UIStyle.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "Alverion"

namespace client
{
	/**
	 * The art, as a file rather than as a texture asset.
	 * A uasset cannot be used by the movie player at all, and that screen covers the load of the first map.
	 * A texture out of /Game would be a package load inside the thing that hides package loads.
	 * Slate reads a png off disk on its own, which is how the engine's own splash works.
	 */
	const TCHAR* const loadingImageName = TEXT("Splash/loading.png");

	/** The size the art was drawn at, which is what the brush is told; a Fill slot stretches it to whatever the window is. */
	const FVector2D loadingImageSize(1920.0f, 1080.0f);

	/** Held for the life of the process, because a brush handed to a widget and then dropped is a texture pulled out from under it. */
	TMap<FString, TSharedPtr<FSlateDynamicImageBrush>> brushes;

	/** The one already read, or a new one, or none when there is no such file. */
	TSharedPtr<FSlateDynamicImageBrush> brushFor(const TCHAR* imageName)
	{
		const FString imagePath = FPaths::ProjectContentDir() / imageName;
		if (TSharedPtr<FSlateDynamicImageBrush>* existing = brushes.Find(imagePath))
		{
			return *existing;
		}

		if (!FPaths::FileExists(imagePath))
		{
			return nullptr;
		}

		TSharedPtr<FSlateDynamicImageBrush> brush = MakeShareable(new FSlateDynamicImageBrush(FName(*imagePath), loadingImageSize));
		brushes.Add(imagePath, brush);
		return brush;
	}

	/**
	 * Falls back to a card with a word on it when the file is missing, so a fresh clone still runs.
	 * The loading word is NSLOCTEXT rather than the string table, because the movie player runs this before anything is certain to be loaded.
	 */
	TSharedRef<SWidget> makeScreen(const TCHAR* imageName, const FLinearColor& cardColor, const FText& fallbackText, const FSlateFontInfo& fallbackFont)
	{
		TSharedPtr<FSlateDynamicImageBrush> brush = brushFor(imageName);

		TSharedRef<SOverlay> screen = SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.ColorAndOpacity(FSlateColor(cardColor))
			];

		if (brush.IsValid())
		{
			// Under the movie player the art does not follow a window resize; shown by a game mode it is an ordinary widget and does.
			screen->AddSlot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(brush.Get())
			];

			return screen;
		}

		// A missing file leaves the card, and a word on it only when there is a word worth saying.
		if (!fallbackText.IsEmpty())
		{
			screen->AddSlot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(fallbackText)
				.Font(fallbackFont)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FSlateColor(UIStyle::labelColor))
			];
		}

		return screen;
	}

	/** Black and nothing else, which is all the boot map wants while the settings load and apply. An empty name reads no file. */
	TSharedRef<SWidget> makeBootScreen()
	{
		return makeScreen(TEXT(""), FLinearColor::Black, FText::GetEmpty(), FSlateFontInfo());
	}

	/** The studio's mark, as text on its dark card until there is a logo; shown the ordinary way, never under the movie player, so the menus' font asset is safe here. */
	TSharedRef<SWidget> makeIntroScreen()
	{
		FSlateFontInfo font = UIStyle::font(UIStyle::studioFontSize);
		font.LetterSpacing = UIStyle::studioLetterSpacing;
		return makeScreen(TEXT(""), UIStyle::cardColor, FText::FromString(UIStyle::studioName), font);
	}

	TSharedRef<SWidget> makeLoadingScreen()
	{
		return makeScreen(loadingImageName, FLinearColor::Black, LOCTEXT("loading", "Loading..."), FCoreStyle::GetDefaultFontStyle("Regular", 28));
	}
}

#undef LOCTEXT_NAMESPACE
