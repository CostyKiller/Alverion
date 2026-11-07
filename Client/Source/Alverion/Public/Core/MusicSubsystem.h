#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "MusicSubsystem.generated.h"

class UAudioComponent;

/**
 * The menu theme, kept by the game instance so it plays on across the intro, the loading map and the front end without starting over.
 * The world fades it out, and the front end starts it again on the way back.
 */
UCLASS()
class ALVERION_API UMusicSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	/** Starts the theme, or leaves it running when it already plays. */
	void playMenuTheme(const UObject& worldContext);

	/** Fades the theme out. */
	void stopMenuTheme();

private:
	/** Follows the music volume in the options, at once. */
	void applyVolume();

	UPROPERTY()
	TObjectPtr<UAudioComponent> _theme;
};
