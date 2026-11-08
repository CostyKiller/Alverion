#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Animation/AnimSingleNodeInstanceProxy.h"

#include "FootIkAnimInstance.generated.h"

/** Samples the clip as the stock single-node player does, then bends each leg onto the ground the game thread found under its foot. */
USTRUCT()
struct FFootIkAnimInstanceProxy : public FAnimSingleNodeInstanceProxy
{
	GENERATED_BODY()

	FFootIkAnimInstanceProxy() = default;
	explicit FFootIkAnimInstanceProxy(UAnimInstance* instance);

	virtual bool Evaluate(FPoseContext& output) override;

	/** Left then right: how far the ground under each foot lies above the plane the body stands on, and which way it faces, in component space. */
	float groundHeights[2] = { 0.0f, 0.0f };
	FVector groundNormals[2] = { FVector::UpVector, FVector::UpVector };
};

/**
 * The stock single-node player, which also keeps both feet on uneven ground: each leg bends to the ground under its foot and the pelvis drops for the lower one.
 * Not grounded, the feet ease back to the clip, which is what airborne and swimming bodies want.
 */
UCLASS(Transient, NotBlueprintable)
class ALVERION_API UFootIkAnimInstance : public UAnimSingleNodeInstance
{
	GENERATED_BODY()

public:
	bool grounded = false;

protected:
	virtual void NativeUpdateAnimation(float deltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};
