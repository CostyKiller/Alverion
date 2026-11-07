#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/UObjectGlobals.h"

#include "CardGameModes.generated.h"

class SWidget;

/**
 * The game modes that only hold a full screen card on the engine's empty Entry map while something loads: boot, intro and loading.
 * The card goes straight onto the viewport, since the UI layers belong to the front end these are waiting for.
 */
UCLASS(Abstract)
class ALVERION_API ACardGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ACardGameMode();

protected:
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

	void showCard(const TSharedRef<SWidget>& card);

	/** Opens a map once, however many reasons to leave arrive. */
	void travelOnce(const FString& map, const FString& options);

private:
	TSharedPtr<SWidget> _card;
	bool _travelling = false;
};

/** The first map of all: black, the saved settings applied, then on to the intro, so a player never sees a frame of the defaults. */
UCLASS()
class ALVERION_API ABootGameMode : public ACardGameMode
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
};

/** The studio card, held for at least two seconds while the front end's map loads behind it. */
UCLASS()
class ALVERION_API AIntroGameMode : public ACardGameMode
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

private:
	void frontEndLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result);
	void holdElapsed();
	void leaveWhenReady();

	FTimerHandle _holdTimer;
	bool _frontEndLoaded = false;
	bool _held = false;
};

/** A loading card while the map named by the travel's target option loads, then into that map. */
UCLASS()
class ALVERION_API ALoadingGameMode : public ACardGameMode
{
	GENERATED_BODY()

public:
	/** Travels to a map by way of this loading card. */
	static void travelTo(UObject* worldContext, const FString& mapPackage);

protected:
	virtual void BeginPlay() override;

private:
	void targetLoaded(const FName& packageName, UPackage* package, EAsyncLoadingResult::Type result);

	FString _target;
};
