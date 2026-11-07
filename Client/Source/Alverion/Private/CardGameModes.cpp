#include "CardGameModes.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "LocalizedText.h"
#include "Misc/PackageName.h"
#include "Styling/CoreStyle.h"
#include "TimerManager.h"
#include "UIStyle.h"
#include "UObject/Package.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	const TCHAR* const emptyMap = TEXT("/Engine/Maps/Entry");
	const TCHAR* const frontEndMap = TEXT("/Game/Maps/L_Login");
	const TCHAR* const targetOption = TEXT("target");

	// The least time the studio card is up. A greeting that comes and goes inside a frame reads as a glitch rather than a studio.
	constexpr float introHoldSeconds = 2.0f;

	constexpr int32 loadingFontSize = 28;

	/** The travel options that bring up the loading card and name the map it loads. */
	FString loadingOptions(const FString& mapPackage)
	{
		return FString::Printf(TEXT("game=/Script/Alverion.LoadingGameMode?%s=%s"), targetOption, *mapPackage);
	}

	/** One colour over the whole viewport, with a line of text centred on it when there is one. */
	TSharedRef<SWidget> makeCard(const FLinearColor& color, const FText& text, const FSlateFontInfo& font)
	{
		TSharedRef<SOverlay> card = SNew(SOverlay)
			+ SOverlay::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SImage)
				.Image(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.ColorAndOpacity(FSlateColor(color))
			];

		if (!text.IsEmpty())
		{
			card->AddSlot()
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(text)
				.Font(font)
				.Justification(ETextJustify::Center)
				.ColorAndOpacity(FSlateColor(UIStyle::labelColor))
			];
		}

		return card;
	}
}

ACardGameMode::ACardGameMode()
{
	DefaultPawnClass = nullptr;
}

void ACardGameMode::EndPlay(const EEndPlayReason::Type reason)
{
	// Taken off by hand, since the viewport outlives this world and would otherwise keep drawing the card.
	UGameViewportClient* viewport = (GetWorld() != nullptr) ? GetWorld()->GetGameViewport() : nullptr;
	if ((viewport != nullptr) && _card.IsValid())
	{
		viewport->RemoveViewportWidgetContent(_card.ToSharedRef());
	}

	_card.Reset();
	Super::EndPlay(reason);
}

void ACardGameMode::showCard(const TSharedRef<SWidget>& card)
{
	UGameViewportClient* viewport = (GetWorld() != nullptr) ? GetWorld()->GetGameViewport() : nullptr;
	if (viewport != nullptr)
	{
		_card = card;
		viewport->AddViewportWidgetContent(card);
	}
}

void ACardGameMode::travelOnce(const FString& map, const FString& options)
{
	if (_travelling)
	{
		return;
	}

	_travelling = true;
	UGameplayStatics::OpenLevel(this, FName(*map), true, options);
}

void ABootGameMode::BeginPlay()
{
	Super::BeginPlay();

	showCard(makeCard(FLinearColor::Black, FText::GetEmpty(), FSlateFontInfo()));

	// Applied on the first map with a window, so the window is the player's own before anything of the game is on it.
	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->ApplySettings(false);
	}

	travelOnce(emptyMap, TEXT("game=/Script/Alverion.IntroGameMode"));
}

void AIntroGameMode::BeginPlay()
{
	Super::BeginPlay();

	FSlateFontInfo font = UIStyle::font(UIStyle::studioFontSize);
	font.LetterSpacing = UIStyle::studioLetterSpacing;
	showCard(makeCard(UIStyle::cardColor, FText::FromString(UIStyle::studioName), font));

	// The front end's map loads behind the card, so the travel at the end finds it already in memory.
	LoadPackageAsync(frontEndMap, FLoadPackageAsyncDelegate::CreateUObject(this, &AIntroGameMode::frontEndLoaded));
	GetWorldTimerManager().SetTimer(_holdTimer, this, &AIntroGameMode::holdElapsed, introHoldSeconds, false);
}

void AIntroGameMode::EndPlay(const EEndPlayReason::Type reason)
{
	GetWorldTimerManager().ClearTimer(_holdTimer);
	Super::EndPlay(reason);
}

void AIntroGameMode::frontEndLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result)
{
	// Counted as loaded either way: a failed preload just loads the slow way inside the travel, and a client stuck on the card is worse.
	if (result != EAsyncLoadingResult::Succeeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("The intro could not preload %s, so the front end loads the slow way."), *packageName.ToString());
	}

	_frontEndLoaded = true;
	leaveWhenReady();
}

void AIntroGameMode::holdElapsed()
{
	_held = true;

	// A front end still loading once the card has had its time is a long load, and the loading card says so in words rather than leaving the studio card up.
	if (!_frontEndLoaded)
	{
		travelOnce(emptyMap, loadingOptions(frontEndMap));
		return;
	}

	leaveWhenReady();
}

void AIntroGameMode::leaveWhenReady()
{
	if (_frontEndLoaded && _held)
	{
		travelOnce(frontEndMap, FString());
	}
}

void ALoadingGameMode::travelTo(UObject* worldContext, const FString& mapPackage)
{
	UGameplayStatics::OpenLevel(worldContext, FName(emptyMap), true, loadingOptions(mapPackage));
}

void ALoadingGameMode::BeginPlay()
{
	Super::BeginPlay();

	showCard(makeCard(FLinearColor::Black, LocalizedText::get(TEXT("menu.loading")), UIStyle::font(loadingFontSize)));

	_target = UGameplayStatics::ParseOption(OptionsString, targetOption);
	if (_target.IsEmpty() || !FPackageName::DoesPackageExist(_target))
	{
		// A wiring fault in whoever travelled here. The front end is somewhere a player can act from, a loading card is not.
		UE_LOG(LogTemp, Warning, TEXT("The loading card has no map to load in options %s, so it returns to the front end."), *OptionsString);
		travelOnce(frontEndMap, FString());
		return;
	}

	LoadPackageAsync(_target, FLoadPackageAsyncDelegate::CreateUObject(this, &ALoadingGameMode::targetLoaded));
}

void ALoadingGameMode::targetLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result)
{
	if (result != EAsyncLoadingResult::Succeeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("The loading card could not preload %s, so it loads the slow way."), *packageName.ToString());
	}

	travelOnce(_target, FString());
}
