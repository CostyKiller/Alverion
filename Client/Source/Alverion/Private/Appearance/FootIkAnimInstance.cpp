#include "Appearance/FootIkAnimInstance.h"

#include "BonePose.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "TwoBoneIK.h"

namespace
{
	// How far above and below the body's feet a foot looks for ground.
	constexpr double footIkReachCm = 45.0;

	// The share of the step between the two feet's ground the pelvis sinks below the lower foot, so neither leg stands locked straight on a slope.
	constexpr float footIkSoftKnees = 0.3f;

	/** Invalid when the skeleton lacks the bone or the current level of detail drops it. */
	FCompactPoseBoneIndex footIkBone(const FBoneContainer& bones, const FName& name)
	{
		const int32 meshIndex = bones.GetPoseBoneIndexForBoneName(name);
		return (meshIndex == INDEX_NONE) ? FCompactPoseBoneIndex(INDEX_NONE) : bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(meshIndex));
	}
}

FFootIkAnimInstanceProxy::FFootIkAnimInstanceProxy(UAnimInstance* instance)
	: FAnimSingleNodeInstanceProxy(instance)
{
}

bool FFootIkAnimInstanceProxy::Evaluate(FPoseContext& output)
{
	FAnimSingleNodeInstanceProxy::Evaluate(output);

	// Level ground leaves the clip as it was sampled.
	bool level = true;
	for (int32 side = 0; side < 2; side++)
	{
		level = level && FMath::IsNearlyZero(groundHeights[side], 0.1f) && groundNormals[side].Equals(FVector::UpVector, 0.001);
	}

	static const FName pelvisName(TEXT("pelvis"));
	static const FName legNames[2][4] = { { TEXT("thigh_l"), TEXT("calf_l"), TEXT("foot_l"), TEXT("ball_l") }, { TEXT("thigh_r"), TEXT("calf_r"), TEXT("foot_r"), TEXT("ball_r") } };
	const FBoneContainer& bones = output.Pose.GetBoneContainer();
	const FCompactPoseBoneIndex pelvis = footIkBone(bones, pelvisName);
	if (level || !pelvis.IsValid())
	{
		return true;
	}

	FCSPose<FCompactPose> pose;
	pose.InitPose(output.Pose);

	// The pelvis drops for the lower foot and a little more, so both knees soften and the higher one bends furthest.
	const float pelvisDrop = FMath::Min(groundHeights[0], groundHeights[1]) - (footIkSoftKnees * FMath::Abs(groundHeights[0] - groundHeights[1]));
	FTransform pelvisTransform = pose.GetComponentSpaceTransform(pelvis);
	pelvisTransform.AddToTranslation(FVector(0.0, 0.0, pelvisDrop));
	const FBoneTransform pelvisMove[] = { FBoneTransform(pelvis, pelvisTransform) };
	pose.SafeSetCSBoneTransforms(pelvisMove);

	for (int32 side = 0; side < 2; side++)
	{
		const FCompactPoseBoneIndex hipBone = footIkBone(bones, legNames[side][0]);
		const FCompactPoseBoneIndex kneeBone = footIkBone(bones, legNames[side][1]);
		const FCompactPoseBoneIndex footBone = footIkBone(bones, legNames[side][2]);
		const FCompactPoseBoneIndex ballBone = footIkBone(bones, legNames[side][3]);
		if (!hipBone.IsValid() || !kneeBone.IsValid() || !footBone.IsValid() || !ballBone.IsValid())
		{
			continue;
		}

		FTransform hip = pose.GetComponentSpaceTransform(hipBone);
		FTransform knee = pose.GetComponentSpaceTransform(kneeBone);
		FTransform foot = pose.GetComponentSpaceTransform(footBone);

		// The sole lies along the slope and the ankle keeps the clip's height above its ground measured across the slope, which straight up is that height over the slope's cosine.
		// The knee bends out over the toes: a standing leg is nearly straight, so the slight kink it already has could point anywhere, often sideways.
		const FVector reach = foot.GetLocation() + FVector(0.0, 0.0, groundHeights[side] - pelvisDrop + ((foot.GetLocation().Z - pelvisDrop) * ((1.0 / FMath::Max(groundNormals[side].Z, 0.5)) - 1.0)));
		AnimationCore::SolveTwoBoneIK(hip, knee, foot, knee.GetLocation() + ((pose.GetComponentSpaceTransform(ballBone).GetLocation() - foot.GetLocation()).GetSafeNormal2D() * 50.0), reach, false, 1.0, 1.0);
		foot.SetRotation(FQuat::FindBetweenNormals(FVector::UpVector, groundNormals[side]) * foot.GetRotation());
		const FBoneTransform leg[] = { FBoneTransform(hipBone, hip), FBoneTransform(kneeBone, knee), FBoneTransform(footBone, foot) };
		pose.SafeSetCSBoneTransforms(leg);
	}

	FCSPose<FCompactPose>::ConvertComponentPosesToLocalPoses(MoveTemp(pose), output.Pose);
	return true;
}

FAnimInstanceProxy* UFootIkAnimInstance::CreateAnimInstanceProxy()
{
	return new FFootIkAnimInstanceProxy(this);
}

void UFootIkAnimInstance::NativeUpdateAnimation(float deltaSeconds)
{
	Super::NativeUpdateAnimation(deltaSeconds);

	const USkeletalMeshComponent* mesh = GetSkelMeshComponent();
	const UWorld* world = GetWorld();
	if ((mesh == nullptr) || (mesh->GetOwner() == nullptr) || (world == nullptr))
	{
		return;
	}

	// Neither the body nor the character it rides on is ground.
	FCollisionQueryParams query(SCENE_QUERY_STAT(FootIk));
	query.AddIgnoredActor(mesh->GetOwner());
	query.AddIgnoredActor(mesh->GetOwner()->GetAttachParentActor());

	// Traced where each foot stood last frame; the height only matters under it, so last frame's place is close enough.
	// Applied at once, with no easing: the clip itself switches at once between standing and running, so the legs change with it.
	static const FName footNames[2] = { TEXT("foot_l"), TEXT("foot_r") };
	FFootIkAnimInstanceProxy& proxy = GetProxyOnGameThread<FFootIkAnimInstanceProxy>();
	const FTransform& frame = mesh->GetComponentTransform();
	for (int32 side = 0; side < 2; side++)
	{
		proxy.groundHeights[side] = 0.0f;
		proxy.groundNormals[side] = FVector::UpVector;
		const FVector foot = mesh->GetSocketLocation(footNames[side]);
		FHitResult hit;
		if (grounded && world->LineTraceSingleByChannel(hit, FVector(foot.X, foot.Y, frame.GetLocation().Z + footIkReachCm), FVector(foot.X, foot.Y, frame.GetLocation().Z - footIkReachCm), ECC_WorldStatic, query))
		{
			proxy.groundHeights[side] = static_cast<float>(hit.ImpactPoint.Z - frame.GetLocation().Z);
			proxy.groundNormals[side] = frame.InverseTransformVectorNoScale(hit.ImpactNormal);
		}
	}
}
