#include "FrontEndGameMode.h"

#include "Appearance.h"
#include "AppearanceApplier.h"
#include "Camera/CameraActor.h"
#include "CardGameModes.h"
#include "Camera/CameraComponent.h"
#include "CharacterCreateWidget.h"
#include "CharacterSelectWidget.h"
#include "ClientPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameSettings.h"
#include "IntroWidget.h"
#include "Kismet/GameplayStatics.h"
#include "LoginWidget.h"
#include "RootLayout.h"
#include "SceneSetup.h"
#include "ServerSelectWidget.h"
#include "Sound/SoundBase.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

namespace
{
	const TCHAR* const continentMap = TEXT("/Game/Maps/L_Continent");
	const TCHAR* const menuMusicPath = TEXT("/Game/Audio/Music/MenuTheme.MenuTheme");

	// The tags on the three camera actors in L_Login, one for each kind of screen.
	const FName loginStop(TEXT("FrontEndStop.Login"));
	const FName selectStop(TEXT("FrontEndStop.Select"));
	const FName createStop(TEXT("FrontEndStop.Create"));

	/**
	 * The tags on the two marker actors an avatar can stand on.
	 * Each marker carries the yaw a MetaHuman body wants, the camera's yaw plus ninety, since that mesh sits ninety degrees off its actor.
	 */
	const FName selectSpotTag(TEXT("FrontEndAvatarSpot.Select"));
	const FName createSpotTag(TEXT("FrontEndAvatarSpot.Create"));

	// A blend scaled by how far the camera travels, so a short hop is quick and a long one is not a lurch.
	constexpr float blendSecondsPerMetre = 0.014f;
	constexpr float shortestBlendSeconds = 0.9f;
	constexpr float longestBlendSeconds = 3.2f;

	// The narrowest a zoom goes, which frames a face from where the stops stand.
	constexpr float minimumFieldOfView = 18.0f;

	// The placeholder figure: the engine cylinder is centred on its origin, so it stands half its height above the spot.
	constexpr double placeholderHalfHeightCm = 90.0;
	constexpr double placeholderHeadHeightCm = 160.0;

	// The placeholder's skin, darkened by the chosen tone. The MetaHuman body reads the same index from its own table.
	const FLinearColor placeholderSkin(0.85f, 0.66f, 0.52f);
	constexpr float skinToneShades[] = { 1.00f, 0.85f, 0.70f, 0.55f, 0.42f, 0.30f };
}

AFrontEndGameMode::AFrontEndGameMode()
{
	PlayerControllerClass = AClientPlayerController::StaticClass();
	DefaultPawnClass = nullptr;
}

void AFrontEndGameMode::travelToWorld(UObject* worldContext)
{
#if WITH_EDITOR
	// The continent's terrain plugin cannot run in a game started from the editor executable: reading the map needs its editor library, and that library crashes the game when the world is torn down.
	// So such a run stops here and says so, rather than crashing on the way in or on the way out.
	if (!GIsEditor)
	{
		const TCHAR* const notice = TEXT("The world runs only in a packaged build of the game.");
		UE_LOG(LogTemp, Warning, TEXT("%s"), notice);
		if (GEngine != nullptr)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Yellow, notice);
		}

		return;
	}
#endif

	// By way of the loading card. The continent's world settings name its game mode.
	ALoadingGameMode::travelTo(worldContext, continentMap);
}

void AFrontEndGameMode::BeginPlay()
{
	Super::BeginPlay();

	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->ApplySettings(false);
		settings->onAudioSettingsChanged.AddUObject(this, &AFrontEndGameMode::applyMusicVolume);
	}

	// The bodies and the eye material start loading now, so the first character shown does not stall the screen.
	Appearance::warm();

	APlayerController* controller = UGameplayStatics::GetPlayerController(this, 0);
	if (controller != nullptr)
	{
		controller->SetShowMouseCursor(true);
		controller->SetInputMode(FInputModeUIOnly());
	}

	collectCameraStops();
	focusStop(loginStop, true);
	collectAvatarSpots();
	installAvatarHooks();

	createRootLayout();
	watchMenuLayer();
	startMenuMusic();
	createIntro();
}

void AFrontEndGameMode::EndPlay(const EEndPlayReason::Type endPlayReason)
{
	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->onAudioSettingsChanged.RemoveAll(this);
	}

	Super::EndPlay(endPlayReason);
}

void AFrontEndGameMode::collectCameraStops()
{
	for (TActorIterator<ACameraActor> it(GetWorld()); it; ++it)
	{
		ACameraActor* camera = *it;
		for (const FName& tag : camera->Tags)
		{
			if ((tag == loginStop) || (tag == selectStop) || (tag == createStop))
			{
				_cameraStops.Add(tag, camera);

				// A camera actor letterboxes to sixteen by nine by default, which bars any other window shape.
				camera->GetCameraComponent()->SetConstraintAspectRatio(false);
			}
		}
	}

	if (_cameraStops.Num() < 3)
	{
		UE_LOG(LogTemp, Warning, TEXT("FrontEndGameMode found %d camera stops in the level, so some screens will not move the camera."), _cameraStops.Num());
	}
}

void AFrontEndGameMode::focusStop(const FName& stopTag, bool instant)
{
	ACameraActor* camera = _cameraStops.FindRef(stopTag);
	APlayerController* controller = UGameplayStatics::GetPlayerController(this, 0);
	if ((camera == nullptr) || (controller == nullptr))
	{
		return;
	}

	float blendSeconds = 0.0f;
	if (!instant)
	{
		const AActor* from = controller->GetViewTarget();
		const double centimetres = (from != nullptr) ? FVector::Dist(from->GetActorLocation(), camera->GetActorLocation()) : 0.0;
		blendSeconds = FMath::Clamp((static_cast<float>(centimetres) / 100.0f) * blendSecondsPerMetre, shortestBlendSeconds, longestBlendSeconds);
	}

	controller->SetViewTargetWithBlend(camera, blendSeconds, EViewTargetBlendFunction::VTBlend_Cubic);
}

void AFrontEndGameMode::collectAvatarSpots()
{
	for (TActorIterator<AActor> it(GetWorld()); it; ++it)
	{
		AActor* actor = *it;
		if (actor->ActorHasTag(selectSpotTag))
		{
			_avatarSpots.Add(EAvatarSpot::Select, actor);
		}
		else if (actor->ActorHasTag(createSpotTag))
		{
			_avatarSpots.Add(EAvatarSpot::Create, actor);
		}
	}

	if (_avatarSpots.Num() < 2)
	{
		UE_LOG(LogTemp, Warning, TEXT("FrontEndGameMode found %d avatar spots in the level, so some screens will show nobody."), _avatarSpots.Num());
	}
}

void AFrontEndGameMode::installAvatarHooks()
{
	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	if (subsystem == nullptr)
	{
		return;
	}

	// Weak, because the subsystem outlives this level and a hook left behind must do nothing.
	TWeakObjectPtr<AFrontEndGameMode> weakThis(this);

	subsystem->showAvatar = [weakThis](EAvatarSpot spot, TConstArrayView<uint8> appearance)
	{
		if (AFrontEndGameMode* gameMode = weakThis.Get())
		{
			gameMode->showAvatar(spot, appearance);
		}
	};

	subsystem->clearAvatar = [weakThis]()
	{
		if (AFrontEndGameMode* gameMode = weakThis.Get())
		{
			gameMode->clearAvatar();
		}
	};

	subsystem->turnAvatar = [weakThis](float degrees)
	{
		if (AFrontEndGameMode* gameMode = weakThis.Get())
		{
			gameMode->turnAvatar(degrees);
		}
	};

	subsystem->zoomView = [weakThis](float degrees)
	{
		if (AFrontEndGameMode* gameMode = weakThis.Get())
		{
			gameMode->zoomView(degrees);
		}
	};
}

void AFrontEndGameMode::showAvatar(EAvatarSpot spot, TConstArrayView<uint8> appearance)
{
	AActor* marker = _avatarSpots.FindRef(spot);
	if (marker == nullptr)
	{
		return;
	}

	// Empty or unreadable bytes stand as the defaults, since a character may predate the appearance block.
	Network::FAppearance unpacked;
	Network::FAppearance::unpack(appearance, unpacked);

	// The body may have to be fetched first, and only the newest request is worth standing up when it lands.
	const uint32 request = ++_avatarRequest;
	const TWeakObjectPtr<AFrontEndGameMode> weakThis(this);
	Appearance::requestBodyClass(unpacked, [weakThis, request, spot, unpacked](UClass* bodyClass)
	{
		AFrontEndGameMode* gameMode = weakThis.Get();
		if ((gameMode != nullptr) && (request == gameMode->_avatarRequest))
		{
			gameMode->standAvatar(spot, bodyClass, unpacked);
		}
	});
}

void AFrontEndGameMode::standAvatar(EAvatarSpot spot, UClass* bodyClass, const Network::FAppearance& appearance)
{
	AActor* marker = _avatarSpots.FindRef(spot);
	if (marker == nullptr)
	{
		return;
	}

	// Without the MetaHuman assets, which a fresh clone does not have, a placeholder figure tinted by the skin tone stands in.
	UClass* wanted = (bodyClass != nullptr) ? bodyClass : AStaticMeshActor::StaticClass();
	if ((_avatar != nullptr) && (_avatar->GetClass() != wanted))
	{
		clearAvatar();
	}

	if (_avatar == nullptr)
	{
		FActorSpawnParameters parameters;
		parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		_avatar = GetWorld()->SpawnActor<AActor>(wanted, marker->GetActorLocation(), marker->GetActorRotation(), parameters);
		if (_avatar == nullptr)
		{
			return;
		}
	}

	if (bodyClass != nullptr)
	{
		// A MetaHuman stands on its feet at the marker, which carries the yaw its mesh needs to face the camera.
		_avatar->SetActorLocationAndRotation(marker->GetActorLocation(), marker->GetActorRotation());
		Appearance::apply(*_avatar, appearance);
		return;
	}

	AStaticMeshActor* figure = Cast<AStaticMeshActor>(_avatar);
	figure->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
	figure->SetActorLocationAndRotation(marker->GetActorLocation() + FVector(0.0, 0.0, placeholderHalfHeightCm), marker->GetActorRotation());

	const float shade = skinToneShades[appearance.skinTone % UE_ARRAY_COUNT(skinToneShades)];
	SceneSetup::paintPlaceholderBody(*figure->GetStaticMeshComponent(), placeholderSkin * shade);
}

void AFrontEndGameMode::clearAvatar()
{
	if (_avatar != nullptr)
	{
		_avatar->Destroy();
		_avatar = nullptr;
	}
}

void AFrontEndGameMode::turnAvatar(float degrees)
{
	// The body turns rather than the camera, since the camera stops are placed by hand.
	if (_avatar != nullptr)
	{
		_avatar->AddActorWorldRotation(FRotator(0.0f, degrees, 0.0f));
	}
}

FRotator AFrontEndGameMode::aimAtHead(const ACameraActor& camera) const
{
	if (_avatar == nullptr)
	{
		return camera.GetActorRotation();
	}

	// A MetaHuman's face mesh where there is one, the placeholder's head height otherwise.
	TArray<USkeletalMeshComponent*> meshes;
	_avatar->GetComponents<USkeletalMeshComponent>(meshes);
	for (const USkeletalMeshComponent* mesh : meshes)
	{
		if (mesh->GetName().Contains(TEXT("Face")))
		{
			return (mesh->Bounds.Origin - camera.GetActorLocation()).Rotation();
		}
	}

	const FVector head = _avatar->GetActorLocation() + FVector(0.0, 0.0, placeholderHeadHeightCm - placeholderHalfHeightCm);
	return (head - camera.GetActorLocation()).Rotation();
}

void AFrontEndGameMode::zoomView(float degrees)
{
	APlayerController* controller = UGameplayStatics::GetPlayerController(this, 0);
	ACameraActor* camera = (controller != nullptr) ? Cast<ACameraActor>(controller->GetViewTarget()) : nullptr;
	UCameraComponent* component = (camera != nullptr) ? camera->GetCameraComponent() : nullptr;
	if (component == nullptr)
	{
		return;
	}

	if (_restingFieldOfView <= 0.0f)
	{
		_restingFieldOfView = component->FieldOfView;
		_restingRotation = camera->GetActorRotation();
	}

	// Zero puts the stop back as it was placed.
	if (degrees == 0.0f)
	{
		component->SetFieldOfView(_restingFieldOfView);
		camera->SetActorRotation(_restingRotation);
		_restingFieldOfView = 0.0f;
		return;
	}

	const float wanted = FMath::Clamp(component->FieldOfView + degrees, minimumFieldOfView, _restingFieldOfView);
	component->SetFieldOfView(wanted);

	// The closer the zoom, the more the camera turns toward the face, so narrowing never crops the head out.
	const float span = _restingFieldOfView - minimumFieldOfView;
	const float closeness = (span > KINDA_SMALL_NUMBER) ? ((_restingFieldOfView - wanted) / span) : 0.0f;
	camera->SetActorRotation(FMath::Lerp(_restingRotation, aimAtHead(*camera), closeness));
}

void AFrontEndGameMode::watchMenuLayer()
{
	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	UCommonActivatableWidgetStack* menu = (subsystem != nullptr) ? subsystem->getLayer(EUILayer::Menu) : nullptr;
	if (menu == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FrontEndGameMode found no Menu layer, so the camera will hold the login stop."));
		return;
	}

	menu->OnDisplayedWidgetChanged().AddUObject(this, &AFrontEndGameMode::handleMenuWidgetChanged);
}

void AFrontEndGameMode::handleMenuWidgetChanged(UCommonActivatableWidget* widget)
{
	if (widget == nullptr)
	{
		return;
	}

	if (widget->IsA<UCharacterSelectWidget>())
	{
		focusStop(selectStop, false);
	}
	else if (widget->IsA<UCharacterCreateWidget>())
	{
		focusStop(createStop, false);
	}
	else if (widget->IsA<ULoginWidget>() || widget->IsA<UServerSelectWidget>())
	{
		focusStop(loginStop, false);
		clearAvatar();
	}
}

void AFrontEndGameMode::startMenuMusic()
{
	_menuMusic = LoadObject<USoundBase>(nullptr, menuMusicPath);
	if (_menuMusic == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("FrontEndGameMode could not load %s, so the front end has no music."), menuMusicPath);
		return;
	}

	_musicComponent = UGameplayStatics::SpawnSound2D(this, _menuMusic);
	applyMusicVolume();
}

void AFrontEndGameMode::applyMusicVolume()
{
	const UGameSettings* settings = UGameSettings::get();
	if ((_musicComponent != nullptr) && (settings != nullptr))
	{
		_musicComponent->SetVolumeMultiplier(settings->getEffectiveMusicVolume());
	}
}

void AFrontEndGameMode::createRootLayout()
{
	_rootLayout = CreateWidget<URootLayout>(GetWorld(), URootLayout::StaticClass());
	if (_rootLayout != nullptr)
	{
		_rootLayout->AddToViewport();
	}
}

void AFrontEndGameMode::createIntro()
{
	// The studio card plays once per install, then every later launch goes straight to the login screen.
	const UGameSettings* settings = UGameSettings::get();
	if ((settings != nullptr) && settings->hasPlayedIntro())
	{
		showLogin();
		return;
	}

	_introWidget = CreateWidget<UIntroWidget>(GetWorld(), UIntroWidget::StaticClass());
	if (_introWidget == nullptr)
	{
		showLogin();
		return;
	}

	_introWidget->onIntroFinished.AddDynamic(this, &AFrontEndGameMode::handleIntroFinished);
	_introWidget->AddToViewport(100);
}

void AFrontEndGameMode::handleIntroFinished()
{
	if (_introWidget != nullptr)
	{
		_introWidget->RemoveFromParent();
		_introWidget = nullptr;
	}

	UGameSettings* settings = UGameSettings::get();
	if (settings != nullptr)
	{
		settings->markIntroPlayed();
	}

	showLogin();
}

void AFrontEndGameMode::showLogin()
{
	UGameInstance* gameInstance = GetGameInstance();
	UUISubsystem* subsystem = (gameInstance != nullptr) ? gameInstance->GetSubsystem<UUISubsystem>() : nullptr;
	if (subsystem != nullptr)
	{
		subsystem->pushWidget(EUILayer::Menu, ULoginWidget::StaticClass());
	}
}
