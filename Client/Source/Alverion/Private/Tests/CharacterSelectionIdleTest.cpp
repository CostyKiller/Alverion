#if WITH_DEV_AUTOMATION_TESTS

#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Appearance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "FrontEnd/FrontEndGameMode.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCharacterSelectionIdleTest, "Alverion.Characters.SelectionIdle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCharacterSelectionIdleTest::RunTest(const FString& parameters)
{
	UWorld* world = UWorld::CreateWorld(EWorldType::Game, false);
	GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(world);
	ON_SCOPE_EXIT
	{
		world->EndPlay(EEndPlayReason::EndPlayInEditor);
		world->DestroyWorld(false);
		GEngine->DestroyWorldContext(world);
	};

	AFrontEndGameMode* frontEnd = world->SpawnActor<AFrontEndGameMode>();
	AActor* marker = world->SpawnActor<AActor>();
	frontEnd->_avatarSpots.Add(EAvatarSpot::Select, marker);
	frontEnd->_avatarSpots.Add(EAvatarSpot::Create, marker);
	world->InitializeActorsForPlay(FURL());
	world->SetBegunPlay(true);
	for (const bool male : { false, true })
	{
		Network::FAppearance appearance;
		appearance.male = male;
		const FString name = male ? TEXT("MH_HumanMale") : TEXT("MH_HumanFemale");
		const FString path = FString::Printf(TEXT("/Game/Characters/MetaHumans/Unpacked/%s/BP_%s.BP_%s_C"), *name, *name, *name);
		UClass* avatarClass = LoadClass<AActor>(nullptr, *path);
		if (!TestNotNull(*name, avatarClass))
		{
			return false;
		}

		frontEnd->standAvatar(EAvatarSpot::Select, avatarClass, appearance);
		AActor* avatar = frontEnd->_avatar;
		if (!TestNotNull(TEXT("Selection spawns a preview"), avatar))
		{
			return false;
		}

		USkeletalMeshComponent* body = FindObjectFast<USkeletalMeshComponent>(avatar, TEXT("Body"));
		if (!TestNotNull(TEXT("Preview owns an assembled skeletal Body"), body))
		{
			return false;
		}

		UAnimSingleNodeInstance* player = body->GetSingleNodeInstance();
		if (!TestNotNull(TEXT("Selection uses single-node playback"), player))
		{
			return false;
		}

		UAnimSequence* idle = Cast<UAnimSequence>(player->GetCurrentAsset());
		if (!TestNotNull(TEXT("Selection supplies a body animation sequence"), idle))
		{
			return false;
		}

		TestEqual(TEXT("Selection uses the relaxed standing idle"), idle->GetPathName(), TEXT("/Game/Characters/Animations/UAL/Idle_Selection_Loop.Idle_Selection_Loop"));
		TestTrue(TEXT("Idle loops"), player->IsLooping());
		TestTrue(TEXT("Body post-process is enabled"), !body->GetDisablePostProcessBlueprint());
		TestNotNull(TEXT("Body has an active post-process instance"), body->GetPostProcessInstance());
		body->SetPosition(idle->GetPlayLength() - 0.1f);
		body->TickAnimation(0.2f, false);
		body->RefreshBoneTransforms();
		const float wrapped = body->GetPosition();
		TestTrue(TEXT("Playback advances across the loop boundary"), FMath::IsNearlyEqual(wrapped, 0.1f, 0.01f));
		TestTrue(TEXT("The assembled avatar has begun play"), avatar->HasActorBegunPlay());
		USkeletalMeshComponent* face = FindObjectFast<USkeletalMeshComponent>(avatar, TEXT("Face"));
		if (!TestNotNull(TEXT("The assembled avatar owns a Face"), face))
		{
			return false;
		}

		face->TickAnimation(0.2f, false);
		face->RefreshBoneTransforms();
		for (const FName bone : { FName(TEXT("neck_01")), FName(TEXT("head")) })
		{
			TestTrue(TEXT("Both skeletons contain the shared bone"), (body->GetBoneIndex(bone) != INDEX_NONE) && (face->GetBoneIndex(bone) != INDEX_NONE));
			const FVector bodyLocation = body->GetSocketLocation(bone);
			const FVector faceLocation = face->GetSocketLocation(bone);
			AddInfo(FString::Printf(TEXT("%s %s shared bone locations: Body %s, Face %s."), *name, *bone.ToString(), *bodyLocation.ToString(), *faceLocation.ToString()));
			TestTrue(TEXT("Face and body align at the neck and head"), bodyLocation.Equals(faceLocation, 0.1));
		}

		frontEnd->standAvatar(EAvatarSpot::Select, avatarClass, appearance);
		TestTrue(TEXT("Refreshing a selection preserves the avatar"), frontEnd->_avatar == avatar);
		TestEqual(TEXT("Refreshing a selection preserves the idle phase"), body->GetPosition(), wrapped);
		frontEnd->standAvatar(EAvatarSpot::Create, avatarClass, appearance);
		USkeletalMeshComponent* creationBody = FindObjectFast<USkeletalMeshComponent>(frontEnd->_avatar.Get(), TEXT("Body"));
		if (!TestNotNull(TEXT("Creation owns an assembled skeletal Body"), creationBody))
		{
			return false;
		}

		UAnimSingleNodeInstance* creationPlayer = creationBody->GetSingleNodeInstance();
		TestTrue(TEXT("Creation plays the same relaxed standing idle"), (creationPlayer != nullptr) && (creationPlayer->GetCurrentAsset() == idle));
	}

	return true;
}

#endif
