#include "World/ProxyCharacter.h"

#include "Appearance.h"
#include "Appearance/AppearanceApplier.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "Core/GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "World/SceneSetup.h"
#include "World/Water.h"

namespace
{
	const FLinearColor playerColor(0.35f, 0.6f, 1.0f);
	const FLinearColor townspersonColor(0.95f, 0.8f, 0.25f);
	const FLinearColor monsterColor(0.85f, 0.2f, 0.15f);

	// The body's centre and the label sit above the feet the server reports.
	constexpr float bodyCentreCm = 90.0f;
	constexpr float labelHeightCm = 215.0f;

	// How far above and below a reported height to look for the island.
	constexpr double groundSearchCm = 20000.0;
}

AProxyCharacter::AProxyCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Feet"));

	_body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	_body->SetupAttachment(RootComponent);
	_body->SetRelativeLocation(FVector(0.0f, 0.0f, bodyCentreCm));

	_label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
	_label->SetupAttachment(RootComponent);
	_label->SetRelativeLocation(FVector(0.0f, 0.0f, labelHeightCm));
	_label->SetHorizontalAlignment(EHTA_Center);
	_label->SetWorldSize(24.0f);
}

void AProxyCharacter::setup(const Network::FCharacterSpawn& spawn)
{
	const FLinearColor color = !spawn.isNpc ? playerColor : (spawn.attackable ? monsterColor : townspersonColor);
	SceneSetup::paintPlaceholderBody(*_body, color);

	_label->SetText(FText::FromString(spawn.name));
	_label->SetTextRenderColor(color.ToFColor(true));

	_speed = spawn.runSpeed;
	_swimSpeed = spawn.swimSpeed;
	standAt(spawn.location, spawn.yaw);

	// Players carry an appearance and wear it; a repeated spawn with the same bytes keeps the body already standing.
	Network::FAppearance appearance;
	if (spawn.isNpc || (spawn.appearance == _wornAppearance) || !Network::FAppearance::unpack(spawn.appearance, appearance))
	{
		return;
	}

	_wornAppearance = spawn.appearance;
	if (_metaHuman != nullptr)
	{
		_metaHuman->Destroy();
		_metaHuman = nullptr;
	}

	// The root is the feet, so the body stands on it; the placeholder hides once the body is there to take its place.
	const TWeakObjectPtr<AProxyCharacter> weakThis(this);
	Appearance::attachTo(*this, appearance, FVector::ZeroVector, [weakThis](AActor* body)
	{
		AProxyCharacter* proxy = weakThis.Get();
		if ((proxy == nullptr) || (body == nullptr))
		{
			return;
		}

		proxy->_metaHuman = body;
		proxy->_body->SetVisibility(false);
	});
}

void AProxyCharacter::EndPlay(const EEndPlayReason::Type reason)
{
	// Attached actors outlive their parent, so the body goes with the proxy.
	if (_metaHuman != nullptr)
	{
		_metaHuman->Destroy();
		_metaHuman = nullptr;
	}

	Super::EndPlay(reason);
}

void AProxyCharacter::walkTo(const FVector& destination)
{
	_destination = destination;
	_walking = true;
}

void AProxyCharacter::standAt(const FVector& location, float yaw)
{
	_walking = false;
	SetActorLocation(settle(location));
	SetActorRotation(FRotator(0.0f, yaw, 0.0f));
}

FVector AProxyCharacter::settle(const FVector& location)
{
	FHitResult hit;
	FCollisionQueryParams query;
	query.AddIgnoredActor(this);

	// No island under a place is open sea.
	const FVector above = location + FVector(0.0, 0.0, groundSearchCm);
	const FVector below = location - FVector(0.0, 0.0, groundSearchCm);
	const bool found = GetWorld()->LineTraceSingleByChannel(hit, above, below, ECC_WorldStatic, query);
	_swimming = Water::tooDeep(found ? hit.ImpactPoint.Z : (Water::surfaceCm - groundSearchCm), _swimming);

	// The root is the feet. A swimmer keeps the depth the server reports, no lower than the seabed and no higher than the swimmer line.
	if (_swimming)
	{
		const double seabed = found ? hit.ImpactPoint.Z : (Water::surfaceCm - groundSearchCm);
		return FVector(location.X, location.Y, FMath::Clamp(location.Z, seabed, Water::swimmerFeetCm));
	}

	return found ? hit.ImpactPoint : location;
}

void AProxyCharacter::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	// A player's body runs or swims while it heads for a destination, and stands or treads water otherwise.
	if (_metaHuman != nullptr)
	{
		const Appearance::EMotion moving = _swimming ? Appearance::EMotion::Swim : Appearance::EMotion::Run;
		const Appearance::EMotion still = _swimming ? Appearance::EMotion::SwimIdle : Appearance::EMotion::Idle;
		Appearance::animate(*_metaHuman, _walking ? moving : still);
	}

	if (_walking)
	{
		// A server too old to report a swim speed leaves it zero, and the run speed stands in.
		const float speed = (_swimming && (_swimSpeed > 0.0f)) ? _swimSpeed : _speed;
		const FVector here = GetActorLocation();
		const double step = static_cast<double>(speed * deltaSeconds);

		// On land the ground carries the height; a swimmer heads for the destination's depth too, and tilts into a dive or a rise.
		const FVector toGo = _swimming ? (FVector(_destination.X, _destination.Y, FMath::Min(_destination.Z, Water::swimmerFeetCm)) - here) : FVector(_destination.X - here.X, _destination.Y - here.Y, 0.0);
		if ((toGo.Size() <= step) || (speed <= 0.0f))
		{
			standAt(_destination, GetActorRotation().Yaw);
		}
		else
		{
			const FVector direction = toGo.GetSafeNormal();
			SetActorLocation(settle(here + (direction * step)));
			SetActorRotation(_swimming ? direction.Rotation() : FRotator(0.0f, direction.Rotation().Yaw, 0.0f));
		}
	}

	// The options screen's nameplate switch, read every frame so turning it shows at once.
	const UGameSettings* settings = UGameSettings::get();
	const bool showName = (settings == nullptr) || settings->getShowNameplates();
	if (_label->IsVisible() != showName)
	{
		_label->SetVisibility(showName);
	}

	// The name turns to whoever is looking.
	const APlayerCameraManager* camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (showName && (camera != nullptr))
	{
		const FVector toCamera = camera->GetCameraLocation() - _label->GetComponentLocation();
		_label->SetWorldRotation(FRotator(0.0f, toCamera.Rotation().Yaw, 0.0f));
	}
}
