#include "World/SceneSetup.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace SceneSetup
{
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
