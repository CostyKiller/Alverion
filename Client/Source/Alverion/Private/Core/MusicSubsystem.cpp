#include "Core/MusicSubsystem.h"

#include "Components/AudioComponent.h"
#include "Core/GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
	const TCHAR* const menuThemePath = TEXT("/Game/Audio/Music/MenuTheme.MenuTheme");

	// How long the theme takes to die away once the world takes over.
	constexpr float themeFadeOutSeconds = 1.0f;
}

void UMusicSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
	Super::Initialize(collection);

	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->onAudioSettingsChanged.AddUObject(this, &UMusicSubsystem::applyVolume);
	}
}

void UMusicSubsystem::Deinitialize()
{
	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->onAudioSettingsChanged.RemoveAll(this);
	}

	if (_theme != nullptr)
	{
		_theme->Stop();
		_theme = nullptr;
	}

	Super::Deinitialize();
}

void UMusicSubsystem::playMenuTheme(const UObject& worldContext)
{
	// Already playing, so it runs on rather than starting over. One still dying away is replaced.
	if (_theme != nullptr)
	{
		const EAudioComponentPlayState state = _theme->GetPlayState();
		if ((state == EAudioComponentPlayState::Playing) || (state == EAudioComponentPlayState::FadingIn))
		{
			return;
		}

		_theme->Stop();
	}

	USoundBase* sound = LoadObject<USoundBase>(nullptr, menuThemePath);
	if (sound == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("MusicSubsystem could not load %s, so there is no menu music."), menuThemePath);
		return;
	}

	// Kept across level travel, and kept by this subsystem rather than destroyed when it ends, so a later stop can still reach it.
	_theme = UGameplayStatics::SpawnSound2D(&worldContext, sound, 1.0f, 1.0f, 0.0f, nullptr, true, false);
	if (_theme == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("MusicSubsystem loaded the menu theme but got no audio component back, so nothing is playing."));
		return;
	}

	applyVolume();

	// Reports what reached the component rather than what was asked for.
	// A missing line is a failed load, a zero is the volume, a number with nothing audible is the device.
	UE_LOG(LogTemp, Display, TEXT("MusicSubsystem started the menu theme at volume %.2f."), _theme->VolumeMultiplier);
}

void UMusicSubsystem::stopMenuTheme()
{
	if (_theme != nullptr)
	{
		_theme->FadeOut(themeFadeOutSeconds, 0.0f);
	}
}

void UMusicSubsystem::applyVolume()
{
	const UGameSettings* settings = UGameSettings::get();
	if ((_theme != nullptr) && (settings != nullptr))
	{
		_theme->SetVolumeMultiplier(settings->getEffectiveMusicVolume());
	}
}
