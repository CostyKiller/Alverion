#include "ProxyCharacter.h"

#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/World.h"
#include "GameSettings.h"
#include "Kismet/GameplayStatics.h"
#include "SceneSetup.h"

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
	standAt(spawn.location, spawn.yaw);
}

void AProxyCharacter::walkTo(const FVector& destination)
{
	_destination = destination;
	_walking = true;
}

void AProxyCharacter::standAt(const FVector& location, float yaw)
{
	_walking = false;
	SetActorLocation(onGround(location));
	SetActorRotation(FRotator(0.0f, yaw, 0.0f));
}

FVector AProxyCharacter::onGround(const FVector& location) const
{
	FHitResult hit;
	FCollisionQueryParams query;
	query.AddIgnoredActor(this);

	const FVector above = location + FVector(0.0, 0.0, groundSearchCm);
	const FVector below = location - FVector(0.0, 0.0, groundSearchCm);
	if (GetWorld()->LineTraceSingleByChannel(hit, above, below, ECC_WorldStatic, query))
	{
		return hit.ImpactPoint;
	}

	return location;
}

void AProxyCharacter::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (_walking)
	{
		const FVector here = GetActorLocation();
		const FVector toGo = FVector(_destination.X - here.X, _destination.Y - here.Y, 0.0);
		const double step = static_cast<double>(_speed * deltaSeconds);

		if ((toGo.Size2D() <= step) || (_speed <= 0.0f))
		{
			standAt(_destination, GetActorRotation().Yaw);
		}
		else
		{
			const FVector direction = toGo.GetSafeNormal2D();
			SetActorLocation(onGround(here + (direction * step)));
			SetActorRotation(direction.Rotation());
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
