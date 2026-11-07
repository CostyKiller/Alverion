#include "SceneSetup.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/SkyLight.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace SceneSetup
{
void buildDaylight(UWorld& world)
{
	FActorSpawnParameters parameters;
	parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ADirectionalLight* sun = world.SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-40.0f, -35.0f, 0.0f), parameters);
	if (sun != nullptr)
	{
		UDirectionalLightComponent* light = Cast<UDirectionalLightComponent>(sun->GetLightComponent());
		light->SetMobility(EComponentMobility::Movable);
		light->SetIntensity(8.0f);
		light->SetAtmosphereSunLight(true);
	}

	AActor* sky = world.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, parameters);
	if (sky != nullptr)
	{
		USkyAtmosphereComponent* atmosphere = NewObject<USkyAtmosphereComponent>(sky);
		sky->SetRootComponent(atmosphere);
		atmosphere->RegisterComponent();
	}

	ASkyLight* skyLight = world.SpawnActor<ASkyLight>(FVector::ZeroVector, FRotator::ZeroRotator, parameters);
	if (skyLight != nullptr)
	{
		// Movable and captured in real time, because a stationary sky light needs a baked capture and an unbaked one lights nothing.
		USkyLightComponent* light = skyLight->GetLightComponent();
		light->SetMobility(EComponentMobility::Movable);
		light->SetRealTimeCapture(true);
		light->RecaptureSky();
	}

	AExponentialHeightFog* fog = world.SpawnActor<AExponentialHeightFog>(FVector::ZeroVector, FRotator::ZeroRotator, parameters);
	if (fog != nullptr)
	{
		fog->GetComponent()->SetFogDensity(0.002f);
	}
}

void paintPlaceholderBody(UStaticMeshComponent& body, const FLinearColor& color)
{
	// The engine's cylinder is a metre wide and a metre tall, centred on its origin.
	static constexpr double widthScale = 0.6;
	static constexpr double heightScale = 1.8;

	UStaticMesh* cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	UMaterialInterface* shape = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if ((cylinder == nullptr) || (shape == nullptr))
	{
		return;
	}

	body.SetStaticMesh(cylinder);
	body.SetRelativeScale3D(FVector(widthScale, widthScale, heightScale));
	body.SetCollisionEnabled(ECollisionEnabled::NoCollision);

	UMaterialInstanceDynamic* tinted = UMaterialInstanceDynamic::Create(shape, &body);
	tinted->SetVectorParameterValue(TEXT("Color"), color);
	body.SetMaterial(0, tinted);
}
}
