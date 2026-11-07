#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameUserSettings.h"

#include "GameSettings.generated.h"

// Window modes the options screen offers. Exclusive fullscreen is deliberately absent: it hands the display
// to the driver, which is what causes the washed out gamma and the darkening seen on 4K and HDR monitors.
// Borderless windowed covers the monitor without any of that.
UENUM(BlueprintType)
enum class EClientWindowMode : uint8
{
	Windowed,
	BorderlessFullscreen
};

// Where the camera sits.
// Third person is the default because the game is built around seeing your character.
UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	ThirdPerson,
	FirstPerson
};

// How the image is anti-aliased and upscaled, which on a deferred renderer are the same choice.
// The values are r.AntiAliasingMethod's own, so the cvar takes this enum unchanged.
// MSAA is absent because it needs the forward renderer, and FSR because its plugin is not enabled.
UENUM(BlueprintType)
enum class EUpscalerMethod : uint8
{
	None = 0,
	Fxaa = 1,
	Taa = 2,
	Tsr = 4,
	Smaa = 5
};

// Raised whenever volumes are applied, so anything already playing can follow the sliders.
DECLARE_MULTICAST_DELEGATE(FOnAudioSettingsChanged);

// Every setting the game persists, engine ones through UGameUserSettings and the rest as config keys here.
// Saved to GameUserSettings.ini, so a corrupt or missing file falls back to the defaults below.
UCLASS(config = GameUserSettings, configdonotcheckdefaults)
class ALVERION_API UGameSettings : public UGameUserSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Settings")
	static UGameSettings* get();

	// UGameUserSettings overrides.
	virtual void SetToDefaults() override;
	virtual void ApplySettings(bool bCheckForCommandLineOverrides) override;

	// Resolutions the monitor actually reports, deduplicated, sorted, larger than 1024x768 and never above the
	// desktop resolution. Anything smaller cannot lay out this UI and anything larger cannot be displayed.
	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	static TArray<FIntPoint> getSupportedResolutions();

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	static FIntPoint getNativeResolution();

	// Refresh rate the monitor reports, used to offer sensible frame rate caps.
	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	static int32 getNativeRefreshRate();

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setClientWindowMode(EClientWindowMode windowMode);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	EClientWindowMode getClientWindowMode() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setFieldOfView(float degrees);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	float getFieldOfView() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setShowFps(bool show);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	bool getShowFps() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setShowStats(bool show);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	bool getShowStats() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setUpscalerMethod(EUpscalerMethod method);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	EUpscalerMethod getUpscalerMethod() const;

	// Nanite and Lumen are the two toggles the quality tiers hang off.
	// Settings of their own rather than something a scalability bucket hides.
	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setNaniteEnabled(bool enabled);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	bool getNaniteEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Display")
	void setLumenEnabled(bool enabled);

	UFUNCTION(BlueprintPure, Category = "Settings|Display")
	bool getLumenEnabled() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void setMasterVolume(float volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float getMasterVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void setMusicVolume(float volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float getMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void setSfxVolume(float volume);

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float getSfxVolume() const;

	// Volumes as they should reach a sound: the category slider scaled by master.
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float getEffectiveMusicVolume() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float getEffectiveSfxVolume() const;

	// Pushes the current volumes out immediately, so a slider can be heard while it is being dragged.
	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void applyAudioNow();

	FOnAudioSettingsChanged onAudioSettingsChanged;

	UFUNCTION(BlueprintCallable, Category = "Settings|Input")
	void setMouseSensitivity(float sensitivity);

	UFUNCTION(BlueprintPure, Category = "Settings|Input")
	float getMouseSensitivity() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void setCameraMode(ECameraMode mode);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	ECameraMode getCameraMode() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void setShowNameplates(bool show);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool getShowNameplates() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void setShowDamageNumbers(bool show);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool getShowDamageNumbers() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Gameplay")
	void setFilterChatProfanity(bool filter);

	UFUNCTION(BlueprintPure, Category = "Settings|Gameplay")
	bool getFilterChatProfanity() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Interface")
	void setHudScale(float scale);

	UFUNCTION(BlueprintPure, Category = "Settings|Interface")
	float getHudScale() const;

	// Culture code such as "en" or "de". Applying it switches the running culture, so text that is already on
	// screen updates without a restart.
	UFUNCTION(BlueprintCallable, Category = "Settings|Interface")
	void setCultureCode(const FString& code);

	UFUNCTION(BlueprintPure, Category = "Settings|Interface")
	FString getCultureCode() const;

	// Culture codes the game offers. Config driven rather than probed from localization data, so the list is
	// complete before a single string is translated; switching still changes number, date and currency format.
	UFUNCTION(BlueprintPure, Category = "Settings|Interface")
	static TArray<FString> getAvailableCultures();

	// The language name in its own language, for example "Deutsch" for de, so a lost player can find their own.
	UFUNCTION(BlueprintPure, Category = "Settings|Interface")
	static FString getCultureDisplayName(const FString& code);

	// True once the company logo has played through, which is what lets every later launch skip it.
	UFUNCTION(BlueprintCallable, Category = "Settings|Interface")
	void markIntroPlayed();

	UFUNCTION(BlueprintPure, Category = "Settings|Interface")
	bool hasPlayedIntro() const;

	// The server last chosen from the list, so the next login can go straight there.
	// Zero means none has been chosen, which a fresh install and a cleared setting share.
	UFUNCTION(BlueprintCallable, Category = "Settings|Interface")
	void rememberServer(int32 serverId);

	// The character the screen opens on, kept across runs the way the server is.
	// Written when one is created and again when one is entered, so returning lands on whoever was played last rather than on whoever happens to be first in the list.
	// Zero means none is remembered, which is what a fresh install and a deleted character both look like.
	void rememberCharacter(uint64 objectId);
	uint64 getRememberedCharacter() const;

	UFUNCTION(BlueprintPure, Category = "Settings|Interface")
	int32 getRememberedServer() const;

	// Player facing ranges for the sliders.
	static constexpr float minFieldOfView = 30.0f;
	static constexpr float maxFieldOfView = 110.0f;
	static constexpr float minMouseSensitivity = 0.01f;
	static constexpr float maxMouseSensitivity = 10.0f;
	static constexpr float minHudScale = 0.5f;
	static constexpr float maxHudScale = 2.0f;

private:
	// Anything below 1025x769 cannot lay out this UI, so it never reaches the resolution list.
	static constexpr int32 minResolutionWidth = 1025;
	static constexpr int32 minResolutionHeight = 769;

	// Set the first time the intro finishes, so a reinstall shows the logo once and never nags again.
	UPROPERTY(config)
	bool introPlayed = false;

	// Int32 rather than the uint16 the wire carries.
	// A config property has to be a type the reflection system stores, and uint16 is not one.
	// The range is checked where it is read, against the list the server just sent.
	UPROPERTY(config)
	int32 lastServerId = 0;

	// Held as a string because an object id is 64 bits and the config property types stop at 32.
	// Empty is none, which keeps a fresh install and a deleted character on the same path.
	UPROPERTY(config)
	FString lastCharacterId;

	// Owed: consumers. Field of view and mouse sensitivity are stored and offered, but nothing reads them until
	// the camera and mouse look arrive at C4, and hud scale waits on the HUD itself.
	UPROPERTY(config)
	float fieldOfView = 90.0f;

	UPROPERTY(config)
	float mouseSensitivity = 0.3f;

	UPROPERTY(config)
	float masterVolume = 0.7f;

	UPROPERTY(config)
	float musicVolume = 0.3f;

	UPROPERTY(config)
	float sfxVolume = 0.7f;

	UPROPERTY(config)
	float hudScale = 1.0f;

	UPROPERTY(config)
	bool showFps = false;

	UPROPERTY(config)
	bool showStats = false;

	// Owed: consumers. The camera arrives at C4, nameplates and damage numbers with combat at C5,
	// and the chat filter once there is a chat window to filter.
	// Stored and offered now because a setting the player cannot find is worse than one that waits.
	UPROPERTY(config)
	ECameraMode cameraMode = ECameraMode::ThirdPerson;

	UPROPERTY(config)
	bool showNameplates = true;

	UPROPERTY(config)
	bool showDamageNumbers = true;

	UPROPERTY(config)
	bool filterChatProfanity = true;

	// Defaults match what DefaultEngine.ini already ships, so a fresh install's toggles read as what is running.
	// Lumen is off there because global illumination belongs to the high tiers only.
	UPROPERTY(config)
	EUpscalerMethod upscalerMethod = EUpscalerMethod::Tsr;

	UPROPERTY(config)
	bool naniteEnabled = true;

	UPROPERTY(config)
	bool lumenEnabled = false;

	UPROPERTY(config)
	FString cultureCode = TEXT("en");

	// The eleven cultures the translations ship in, written as their ICU codes.
	UPROPERTY(config)
	TArray<FString> supportedCultures = { TEXT("en"), TEXT("de"), TEXT("el"), TEXT("es"), TEXT("fr"), TEXT("it"), TEXT("ja"), TEXT("ko"), TEXT("pt"), TEXT("ru"), TEXT("zh") };

	void applyAudioSettings();
	void applyCultureSetting();
	void applyStatSettings();
	void applyRenderSettings();
};
