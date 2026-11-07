#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "ScreenshotSubsystem.generated.h"

class IInputProcessor;

/**
 * F12 on every screen, the login and character screens as well as the world: the game view, interface and all, saved to the game folder's Screenshots.
 * Where the game folder cannot be written the shots go to the player's Documents/Alverion/Screenshots instead, and a toast names each file once it is on disk.
 * The key is read ahead of every screen, since the front end keeps its keys from the game.
 */
UCLASS()
class ALVERION_API UScreenshotSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;
	virtual void Deinitialize() override;

	void takeScreenshot();

private:
	/** The game folder's Screenshots, or the player's Documents/Alverion/Screenshots where the game folder cannot be written. Chosen once. */
	FString screenshotFolder();

	/** The engine has written the shot, so the toast comes up now and is never in the picture. */
	void handleScreenshotProcessed();

	TSharedPtr<IInputProcessor> _keyReader;

	FString _screenshotFolder;

	/** The file name of a shot asked for and not yet on disk, empty otherwise. */
	FString _pendingScreenshotName;
};
