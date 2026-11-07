#include "GameSettings.h"

#include "AudioDevice.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GenericPlatform/GenericApplication.h"
#include "HAL/IConsoleManager.h"
#include "Internationalization/Culture.h"
#include "Internationalization/Internationalization.h"
#include "RHI.h"

UGameSettings* UGameSettings::get()
{
	return Cast<UGameSettings>(UGameUserSettings::GetGameUserSettings());
}

void UGameSettings::SetToDefaults()
{
	Super::SetToDefaults();

	fieldOfView = 90.0f;
	mouseSensitivity = 0.3f;
	masterVolume = 0.7f;
	musicVolume = 0.3f;
	sfxVolume = 0.7f;
	hudScale = 1.0f;
	showFps = false;
	showStats = false;
	cultureCode = TEXT("en");
	cameraMode = ECameraMode::ThirdPerson;
	showNameplates = true;
	showDamageNumbers = true;
	filterChatProfanity = true;
	upscalerMethod = EUpscalerMethod::Tsr;
	naniteEnabled = true;
	lumenEnabled = false;

	// Borderless is the default because exclusive fullscreen is never offered, see EClientWindowMode.
	SetFullscreenMode(EWindowMode::WindowedFullscreen);

	const FIntPoint nativeResolution = getNativeResolution();
	if (nativeResolution.X > 0 && nativeResolution.Y > 0)
	{
		SetScreenResolution(nativeResolution);
	}
}

void UGameSettings::ApplySettings(bool bCheckForCommandLineOverrides)
{
	Super::ApplySettings(bCheckForCommandLineOverrides);

	applyAudioSettings();
	applyCultureSetting();
	applyStatSettings();
	applyRenderSettings();
}

TArray<FIntPoint> UGameSettings::getSupportedResolutions()
{
	TArray<FIntPoint> results;
	const FIntPoint nativeResolution = getNativeResolution();

	FScreenResolutionArray resolutions;
	if (!RHIGetAvailableResolutions(resolutions, true))
	{
		UE_LOG(LogTemp, Warning, TEXT("GameSettings could not read the display modes, falling back to the desktop resolution."));
		results.Add(nativeResolution);
		return results;
	}

	for (const FScreenResolutionRHI& resolution : resolutions)
	{
		const FIntPoint candidate = FIntPoint(static_cast<int32>(resolution.Width), static_cast<int32>(resolution.Height));

		if (candidate.X < minResolutionWidth || candidate.Y < minResolutionHeight)
		{
			continue;
		}

		// A mode larger than the desktop cannot be displayed, so offering it only produces a broken window.
		if (nativeResolution.X > 0 && (candidate.X > nativeResolution.X || candidate.Y > nativeResolution.Y))
		{
			continue;
		}

		// The RHI lists one entry per refresh rate, so the same size arrives several times.
		results.AddUnique(candidate);
	}

	if (results.IsEmpty())
	{
		results.Add(nativeResolution);
	}

	results.Sort([](const FIntPoint& left, const FIntPoint& right)
	{
		return (left.X != right.X) ? (left.X < right.X) : (left.Y < right.Y);
	});

	return results;
}

FIntPoint UGameSettings::getNativeResolution()
{
	FDisplayMetrics displayMetrics;
	FDisplayMetrics::RebuildDisplayMetrics(displayMetrics);

	return FIntPoint(displayMetrics.PrimaryDisplayWidth, displayMetrics.PrimaryDisplayHeight);
}

int32 UGameSettings::getNativeRefreshRate()
{
	FScreenResolutionArray resolutions;
	if (!RHIGetAvailableResolutions(resolutions, true))
	{
		return 60;
	}

	const FIntPoint nativeResolution = getNativeResolution();
	int32 highestRate = 0;

	for (const FScreenResolutionRHI& resolution : resolutions)
	{
		if (static_cast<int32>(resolution.Width) == nativeResolution.X && static_cast<int32>(resolution.Height) == nativeResolution.Y)
		{
			highestRate = FMath::Max(highestRate, static_cast<int32>(resolution.RefreshRate));
		}
	}

	return (highestRate > 0) ? highestRate : 60;
}

void UGameSettings::setClientWindowMode(EClientWindowMode windowMode)
{
	SetFullscreenMode((windowMode == EClientWindowMode::BorderlessFullscreen) ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed);
}

EClientWindowMode UGameSettings::getClientWindowMode() const
{
	// Exclusive fullscreen can still arrive from an edited ini or a command line, and it reads as borderless
	// here rather than adding a third entry to a list that deliberately has two.
	return (GetFullscreenMode() == EWindowMode::Windowed) ? EClientWindowMode::Windowed : EClientWindowMode::BorderlessFullscreen;
}

void UGameSettings::setFieldOfView(float degrees)
{
	fieldOfView = FMath::Clamp(degrees, minFieldOfView, maxFieldOfView);
}

float UGameSettings::getFieldOfView() const
{
	return fieldOfView;
}

void UGameSettings::setShowFps(bool show)
{
	showFps = show;
}

bool UGameSettings::getShowFps() const
{
	return showFps;
}

void UGameSettings::setShowStats(bool show)
{
	showStats = show;
}

bool UGameSettings::getShowStats() const
{
	return showStats;
}

void UGameSettings::setMasterVolume(float volume)
{
	masterVolume = FMath::Clamp(volume, 0.0f, 1.0f);
}

float UGameSettings::getMasterVolume() const
{
	return masterVolume;
}

void UGameSettings::setMusicVolume(float volume)
{
	musicVolume = FMath::Clamp(volume, 0.0f, 1.0f);
}

float UGameSettings::getMusicVolume() const
{
	return musicVolume;
}

void UGameSettings::setSfxVolume(float volume)
{
	sfxVolume = FMath::Clamp(volume, 0.0f, 1.0f);
}

float UGameSettings::getSfxVolume() const
{
	return sfxVolume;
}

void UGameSettings::applyAudioNow()
{
	applyAudioSettings();
}

float UGameSettings::getEffectiveMusicVolume() const
{
	return masterVolume * musicVolume;
}

float UGameSettings::getEffectiveSfxVolume() const
{
	return masterVolume * sfxVolume;
}

void UGameSettings::setMouseSensitivity(float sensitivity)
{
	mouseSensitivity = FMath::Clamp(sensitivity, minMouseSensitivity, maxMouseSensitivity);
}

float UGameSettings::getMouseSensitivity() const
{
	return mouseSensitivity;
}

void UGameSettings::setHudScale(float scale)
{
	hudScale = FMath::Clamp(scale, minHudScale, maxHudScale);
}

float UGameSettings::getHudScale() const
{
	return hudScale;
}

void UGameSettings::setUpscalerMethod(EUpscalerMethod method)
{
	upscalerMethod = method;
}

EUpscalerMethod UGameSettings::getUpscalerMethod() const
{
	return upscalerMethod;
}

void UGameSettings::setNaniteEnabled(bool enabled)
{
	naniteEnabled = enabled;
}

bool UGameSettings::getNaniteEnabled() const
{
	return naniteEnabled;
}

void UGameSettings::setLumenEnabled(bool enabled)
{
	lumenEnabled = enabled;
}

bool UGameSettings::getLumenEnabled() const
{
	return lumenEnabled;
}

void UGameSettings::setCameraMode(ECameraMode mode)
{
	cameraMode = mode;
}

ECameraMode UGameSettings::getCameraMode() const
{
	return cameraMode;
}

void UGameSettings::setShowNameplates(bool show)
{
	showNameplates = show;
}

bool UGameSettings::getShowNameplates() const
{
	return showNameplates;
}

void UGameSettings::setShowDamageNumbers(bool show)
{
	showDamageNumbers = show;
}

bool UGameSettings::getShowDamageNumbers() const
{
	return showDamageNumbers;
}

void UGameSettings::setFilterChatProfanity(bool filter)
{
	filterChatProfanity = filter;
}

bool UGameSettings::getFilterChatProfanity() const
{
	return filterChatProfanity;
}

void UGameSettings::setCultureCode(const FString& code)
{
	cultureCode = code.IsEmpty() ? TEXT("en") : code;
}

FString UGameSettings::getCultureCode() const
{
	return cultureCode;
}

void UGameSettings::markIntroPlayed()
{
	if (introPlayed)
	{
		return;
	}

	introPlayed = true;

	// Written on the spot rather than with the rest of the options,
	// because a flag whose run never reaches a normal save would show the logo again on every launch.
	SaveSettings();
}

bool UGameSettings::hasPlayedIntro() const
{
	return introPlayed;
}

void UGameSettings::rememberServer(int32 serverId)
{
	if (lastServerId == serverId)
	{
		return;
	}

	lastServerId = serverId;

	// Written on the spot, for the reason the intro flag gives.
	// A choice made in a run that then crashes is a choice the player would have to make again.
	SaveSettings();
}

int32 UGameSettings::getRememberedServer() const
{
	return lastServerId;
}

void UGameSettings::rememberCharacter(uint64 objectId)
{
	const FString written = (objectId != 0) ? LexToString(objectId) : FString();
	if (lastCharacterId == written)
	{
		return;
	}

	lastCharacterId = written;

	// Written on the spot, for the reason the remembered server gives.
	// A character made in a run that then crashes is one the player would come back to and not find.
	SaveSettings();
}

uint64 UGameSettings::getRememberedCharacter() const
{
	uint64 objectId = 0;
	LexFromString(objectId, *lastCharacterId);

	return objectId;
}

TArray<FString> UGameSettings::getAvailableCultures()
{
	const UGameSettings* settings = get();
	if (settings == nullptr || settings->supportedCultures.IsEmpty())
	{
		return { TEXT("en") };
	}

	TArray<FString> codes = settings->supportedCultures;

	if (!codes.Contains(TEXT("en")))
	{
		codes.Insert(TEXT("en"), 0);
	}

	return codes;
}

FString UGameSettings::getCultureDisplayName(const FString& code)
{
	const FCulturePtr culture = FInternationalization::Get().GetCulture(code);
	if (!culture.IsValid())
	{
		return code;
	}

	return culture->GetNativeName();
}

void UGameSettings::applyAudioSettings()
{
	if (GEngine == nullptr)
	{
		return;
	}

	FAudioDeviceHandle audioDevice = GEngine->GetMainAudioDevice();
	if (!audioDevice.IsValid())
	{
		return;
	}

	// Master rides the whole device. Music and effects are applied per sound by whoever plays them, which is why
	// anything already playing is told to refresh.
	// Owed: a submix graph. Once there is more audio than a menu bed and a click, music and effects belong on
	// their own submixes rather than each sound being told its own volume at play time.
	audioDevice->SetTransientPrimaryVolume(masterVolume);

	onAudioSettingsChanged.Broadcast();
}

void UGameSettings::applyCultureSetting()
{
	// The editor and PIE share one process with the editor's own UI,
	// so switching culture here repaints the entire editor and survives a restart,
	// which is not something a game language setting is allowed to do.
	// A packaged build is also the only place the game's own translations load,
	// so leaving the switch to it costs nothing.
	if (GIsEditor)
	{
		return;
	}

	if (cultureCode.IsEmpty())
	{
		return;
	}

	if (FInternationalization::Get().GetCurrentCulture()->GetName() == cultureCode)
	{
		return;
	}

	if (!FInternationalization::Get().SetCurrentCulture(cultureCode))
	{
		UE_LOG(LogTemp, Warning, TEXT("GameSettings could not switch to culture %s, staying on %s."), *cultureCode, *FInternationalization::Get().GetCurrentCulture()->GetName());
	}
}

void UGameSettings::applyRenderSettings()
{
	// Written through the console variables rather than a scalability bucket,
	// because the buckets set how good each feature looks and these choose which feature runs at all.
	// BaseScalability.ini touches neither r.AntiAliasingMethod nor the two method cvars, so nothing here fights it.
	IConsoleManager& console = IConsoleManager::Get();

	if (IConsoleVariable* antiAliasing = console.FindConsoleVariable(TEXT("r.AntiAliasingMethod")))
	{
		antiAliasing->Set(static_cast<int32>(upscalerMethod), ECVF_SetByGameSetting);
	}

	if (IConsoleVariable* nanite = console.FindConsoleVariable(TEXT("r.Nanite")))
	{
		nanite->Set(naniteEnabled ? 1 : 0, ECVF_SetByGameSetting);
	}

	// Lumen owns both halves or neither. Reflections fall back to screen space rather than to nothing,
	// which is what the project ships with global illumination off.
	if (IConsoleVariable* globalIllumination = console.FindConsoleVariable(TEXT("r.DynamicGlobalIlluminationMethod")))
	{
		globalIllumination->Set(lumenEnabled ? 1 : 0, ECVF_SetByGameSetting);
	}

	if (IConsoleVariable* reflections = console.FindConsoleVariable(TEXT("r.ReflectionMethod")))
	{
		reflections->Set(lumenEnabled ? 1 : 2, ECVF_SetByGameSetting);
	}
}

void UGameSettings::applyStatSettings()
{
	if (GEngine == nullptr)
	{
		return;
	}

	UWorld* world = GEngine->GetCurrentPlayWorld();
	if (world == nullptr)
	{
		return;
	}

	UGameViewportClient* viewport = world->GetGameViewport();
	if (viewport == nullptr)
	{
		return;
	}

	// The stat commands toggle, so they are only issued when the requested state differs from what is showing.
	if (viewport->IsStatEnabled(TEXT("FPS")) != showFps)
	{
		GEngine->Exec(world, TEXT("stat fps"));
	}

	if (viewport->IsStatEnabled(TEXT("Unit")) != showStats)
	{
		GEngine->Exec(world, TEXT("stat unit"));
	}
}
