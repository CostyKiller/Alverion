#include "AppearanceApplier.h"

#include "Appearance.h"
#include "AppearanceChoices.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GroomAsset.h"
#include "GroomBindingAsset.h"
#include "GroomComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/PackageName.h"

namespace
{
	struct FGroom
	{
		/** The kind folder the plugin files the groom under. */
		const TCHAR* category;

		/** The asset name, which the binding baked for each face is named after too. */
		const TCHAR* name;
	};

	/**
	 * Indexed by the hairStyle byte. Thirty four of the plugin's thirty eight: the receding and balding four were pulled on the operator's call,
	 * since one roster serves both sexes. Stored characters carry the byte, so the order only ever grows at the end.
	 */
	const FGroom hairStyles[] =
	{
		{ TEXT("Hair"), TEXT("Hair_S_Pixie") },
		{ TEXT("Hair"), TEXT("Hair_M_BobStraight") },
		{ TEXT("Hair"), TEXT("Hair_L_Straight") },
		{ TEXT("Hair"), TEXT("Hair_M_BobBangs") },
		{ TEXT("Hair"), TEXT("Hair_L_StraightBangs") },
		{ TEXT("Hair"), TEXT("Hair_S_BobLayered") },
		{ TEXT("Hair"), TEXT("Hair_L_MessyClumps") },
		{ TEXT("Hair"), TEXT("Hair_M_BobCurly") },
		{ TEXT("Hair"), TEXT("Hair_M_BobMessy") },
		{ TEXT("Hair"), TEXT("Hair_M_BobSlick") },
		{ TEXT("Hair"), TEXT("Hair_M_TwistedBraids") },
		{ TEXT("Hair"), TEXT("Hair_M_FauxMohawk") },
		{ TEXT("Hair"), TEXT("Hair_S_LowPonytail") },
		{ TEXT("Hair"), TEXT("Hair_S_Updo") },
		{ TEXT("Hair"), TEXT("Hair_S_UpdoBraids") },
		{ TEXT("Hair"), TEXT("Hair_S_UpdoBuns") },
		{ TEXT("Hair"), TEXT("Hair_S_Cornrows") },
		{ TEXT("Hair"), TEXT("Hair_S_Coil") },
		{ TEXT("Hair"), TEXT("Hair_S_CoilBuzzCut") },
		{ TEXT("Hair"), TEXT("Hair_S_BrushCut") },
		{ TEXT("Hair"), TEXT("Hair_L_AfroCurly") },
		{ TEXT("Hair"), TEXT("Hair_M_Layered") },
		{ TEXT("Hair"), TEXT("Hair_M_Mohawk") },
		{ TEXT("Hair"), TEXT("Hair_M_SideSweptFringe") },
		{ TEXT("Hair"), TEXT("Hair_S_SideSweptFringe") },
		{ TEXT("Hair"), TEXT("Hair_S_SweptUp") },
		{ TEXT("Hair"), TEXT("Hair_S_SlickBack") },
		{ TEXT("Hair"), TEXT("Hair_S_PulledBack") },
		{ TEXT("Hair"), TEXT("Hair_S_Casual") },
		{ TEXT("Hair"), TEXT("Hair_S_Messy") },
		{ TEXT("Hair"), TEXT("Hair_S_CurlyFade") },
		{ TEXT("Hair"), TEXT("Hair_S_AfroFade") },
		{ TEXT("Hair"), TEXT("Hair_S_360Waves") },
		{ TEXT("Hair"), TEXT("Hair_S_Clean") }
	};

	/** Indexed by the eyebrows byte: every brow groom with a binding baked for all ten faces. */
	const FGroom eyebrowStyles[] =
	{
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Natural") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_SlightArch") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Fine") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Thin") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_S_FlatThin") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Full") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Dense") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Thick") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_FlatThick") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Wide") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Swept") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Messy") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Unruly") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Slit") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_M_Shaded") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_S_Shaded") },
		{ TEXT("Eyebrows"), TEXT("Eyebrows_L_Shaded") }
	};

	/** Indexed by the beard byte less one, zero being a clean chin. */
	const FGroom beards[] =
	{
		{ TEXT("Beards"), TEXT("Beard_S_Stubble") },
		{ TEXT("Beards"), TEXT("Beard_M_Stubble") },
		{ TEXT("Beards"), TEXT("Beard_S_Uneven") },
		{ TEXT("Beards"), TEXT("Beard_S_PencilThin") },
		{ TEXT("Beards"), TEXT("Beard_S_Full") },
		{ TEXT("Beards"), TEXT("Beard_S_Curly") },
		{ TEXT("Beards"), TEXT("Beard_M_Curly") },
		{ TEXT("Beards"), TEXT("Beard_M_MuttonChops") },
		{ TEXT("Beards"), TEXT("Beard_L_Full") },
		{ TEXT("Beards"), TEXT("Beard_L_Messy") },
		{ TEXT("Beards"), TEXT("Goatee_S_SoulpatchStrip") },
		{ TEXT("Beards"), TEXT("Goatee_M_Soulpatch") },
		{ TEXT("Beards"), TEXT("Goatee_S_ChinStrap") },
		{ TEXT("Beards"), TEXT("Goatee_M_Pointed") },
		{ TEXT("Beards"), TEXT("Goatee_L_Wavy") },
		{ TEXT("Beards"), TEXT("Goatee_L_Scraggly") }
	};

	/** Indexed by the mustache byte less one, zero being a bare lip. */
	const FGroom mustaches[] =
	{
		{ TEXT("Mustaches"), TEXT("Mustache_S_Stubble") },
		{ TEXT("Mustaches"), TEXT("Mustache_M_Stubble") },
		{ TEXT("Mustaches"), TEXT("Mustache_L_Stubble") },
		{ TEXT("Mustaches"), TEXT("Mustache_S_PencilThin") },
		{ TEXT("Mustaches"), TEXT("Mustache_S_Uneven") },
		{ TEXT("Mustaches"), TEXT("Mustache_S_Full") },
		{ TEXT("Mustaches"), TEXT("Mustache_S_Curly") },
		{ TEXT("Mustaches"), TEXT("Mustache_M_Curly") },
		{ TEXT("Mustaches"), TEXT("Mustache_S_ChinStrap") },
		{ TEXT("Mustaches"), TEXT("Mustache_S_Horseshoe") },
		{ TEXT("Mustaches"), TEXT("Mustache_M_Gunslinger") },
		{ TEXT("Mustaches"), TEXT("Mustache_L_Handlebar") },
		{ TEXT("Mustaches"), TEXT("Mustache_L_Full") },
		{ TEXT("Mustaches"), TEXT("Mustache_L_Wavy") },
		{ TEXT("Mustaches"), TEXT("Mustache_L_Messy") },
		{ TEXT("Mustaches"), TEXT("Mustache_L_Scraggly") }
	};

	/** Melanin is how dark, from white at zero to black at one; redness is how red what remains is. Measured on screen, not reasoned. */
	struct FHairColour
	{
		float melanin;
		float redness;
	};

	/** Indexed by the hairColor byte: brown, black, auburn, red, blonde. */
	const FHairColour hairColours[] =
	{
		{ 0.75f, 0.25f },
		{ 1.00f, 0.00f },
		{ 0.45f, 1.00f },
		{ 0.25f, 1.00f },
		{ 0.12f, 0.35f }
	};

	/** The iris hue is a ramp lookup rather than a hue wheel, so these were measured on screen; saturation reaches grey and value reaches black. */
	struct FEyeColour
	{
		float hue;
		float value;
		float saturation;
	};

	/** Indexed by the eyeColor byte: brown, amber, green, blue, grey, black. */
	const FEyeColour eyeColours[] =
	{
		{ 0.90f, 0.56f, 1.70f },
		{ 0.62f, 0.65f, 1.90f },
		{ 0.35f, 0.60f, 1.80f },
		{ 0.08f, 0.62f, 1.80f },
		{ 0.10f, 0.60f, 0.25f },
		{ 0.90f, 0.10f, 0.60f }
	};

	/** Indexed by the skinTone byte: the post-bake value multiplier, one being the skin as built. Above one overflows into red. */
	const float skinTones[] = { 1.00f, 0.85f, 0.70f, 0.55f, 0.42f, 0.30f };

	/** Filed by sex, indexed by the head byte. Whole characters, picked by the operator from the plugin's presets. */
	const TCHAR* const femaleHeads[] = { TEXT("Jelena"), TEXT("Vivian"), TEXT("Aera"), TEXT("Ada") };
	const TCHAR* const maleHeads[] = { TEXT("Victor"), TEXT("Orlando"), TEXT("Kelvin"), TEXT("Cameron") };

	// The screen cycles up to these counts, so a table shorter than its count is a choice that does nothing.
	static_assert(UE_ARRAY_COUNT(hairStyles) == AppearanceChoices::hairStyles, "The hairstyle table and the creation screen's count have to agree.");
	static_assert(UE_ARRAY_COUNT(eyebrowStyles) == AppearanceChoices::eyebrows, "The eyebrow table and the creation screen's count have to agree.");
	static_assert((UE_ARRAY_COUNT(beards) + 1) == AppearanceChoices::beards, "The beard table plus the clean chin have to match the creation screen's count.");
	static_assert((UE_ARRAY_COUNT(mustaches) + 1) == AppearanceChoices::mustaches, "The moustache table plus the bare lip have to match the creation screen's count.");
	static_assert(UE_ARRAY_COUNT(hairColours) == AppearanceChoices::hairColors, "The hair colour table and the creation screen's count have to agree.");
	static_assert(UE_ARRAY_COUNT(eyeColours) == AppearanceChoices::eyeColors, "The eye colour table and the creation screen's count have to agree.");
	static_assert(UE_ARRAY_COUNT(skinTones) == AppearanceChoices::skinTones, "The skin tone table and the creation screen's count have to agree.");
	static_assert(UE_ARRAY_COUNT(femaleHeads) == AppearanceChoices::heads, "The female head table and the creation screen's count have to agree.");
	static_assert(UE_ARRAY_COUNT(maleHeads) == AppearanceChoices::heads, "The male head table and the creation screen's count have to agree.");

	const TCHAR* const charactersFolder = TEXT("/Game/Characters/MetaHumans/Unpacked");
	const TCHAR* const bindingFolder = TEXT("/Game/Characters/MetaHumans/Grooms");
	const TCHAR* const groomFolder = TEXT("/MetaHumanCharacter/Optional/Grooms/GroomAssets");
	const TCHAR* const femaleBaseName = TEXT("MH_HumanFemale");
	const TCHAR* const maleBaseName = TEXT("MH_HumanMale");

	/** The baked eye material ignores colour, so the eye slots get an instance of this parametric one instead. */
	const TCHAR* const parametricEyePath = TEXT("/MetaHumanCharacter/Lookdev_UHM/Eye/Materials/MI_eye_eyeball_unified_MH_preset_left.MI_eye_eyeball_unified_MH_preset_left");

	// Named exactly as the materials name them, spaces and all.
	const FName hairMelaninParameter(TEXT("hairMelanin"));
	const FName hairRednessParameter(TEXT("hairRedness"));
	const FName irisPrimaryHueParameter(TEXT("Iris Primary Color Hue"));
	const FName irisPrimaryValueParameter(TEXT("Iris Primary Color Value"));
	const FName irisSecondaryHueParameter(TEXT("Iris Secondary Color Hue"));
	const FName irisSecondaryValueParameter(TEXT("Iris Secondary Color Value"));
	const FName irisSaturationParameter(TEXT("Iris Global Saturation"));
	const FName skinValueParameter(TEXT("Basecolor Global Value Post-Bake"));

	// Slots by name rather than by material, since the blueprint swaps in dynamic instances named for nothing.
	const FName leftEyeSlot(TEXT("eyeLeft_shader_shader"));
	const FName rightEyeSlot(TEXT("eyeRight_shader_shader"));
	const FName bodySkinSlot(TEXT("body_shader_shader"));
	const FName faceSkinSlots[] = { FName(TEXT("head_LOD1_shader_shader")), FName(TEXT("head_LOD3_shader_shader")), FName(TEXT("head_LOD57_shader_shader")) };

	/** Lashes stay dark on a blonde head, and fuzz is skin to anybody looking. Brows, beard and moustache follow the hair. */
	const TCHAR* const uncolouredGrooms[] = { TEXT("Eyelashes"), TEXT("Fuzz") };

	/** What each groom component was last asked to wear, so a slow fetch cannot overwrite a later fast one. */
	TMap<TWeakObjectPtr<UGroomComponent>, FString> wantedGrooms;

	/** Held until done, because a released handle is a cancelled load. */
	TSharedPtr<FStreamableHandle> warmed;
	TMap<FString, TSharedPtr<FStreamableHandle>> pendingBodies;

	USkeletalMeshComponent* findMesh(AActor& avatar, const TCHAR* part)
	{
		TArray<USkeletalMeshComponent*> meshes;
		avatar.GetComponents<USkeletalMeshComponent>(meshes);
		for (USkeletalMeshComponent* mesh : meshes)
		{
			if (mesh->GetName().Contains(part))
			{
				return mesh;
			}
		}

		return nullptr;
	}

	FString characterPackage(const FString& characterName)
	{
		return FString::Printf(TEXT("%s/%s/BP_%s"), charactersFolder, *characterName, *characterName);
	}

	/** The picked head when it is installed, the base body otherwise, and empty when neither is: a fresh clone has no MetaHumans at all. */
	FString characterNameFor(const Network::FAppearance& appearance)
	{
		const TCHAR* const* heads = appearance.male ? maleHeads : femaleHeads;
		const FString headName = FString::Printf(TEXT("MH_%s"), heads[(appearance.head < AppearanceChoices::heads) ? appearance.head : 0]);
		if (FPackageName::DoesPackageExist(characterPackage(headName)))
		{
			return headName;
		}

		const FString baseName = appearance.male ? maleBaseName : femaleBaseName;
		return FPackageName::DoesPackageExist(characterPackage(baseName)) ? baseName : FString();
	}

	FString bodyClassPath(const FString& characterName)
	{
		return FString::Printf(TEXT("%s.BP_%s_C"), *characterPackage(characterName), *characterName);
	}

	/** One component per slot, found by name because a MetaHuman carries six grooms and the first of a class could be any of them. */
	UGroomComponent* findOrCreateGroom(AActor& avatar, USkeletalMeshComponent& face, const TCHAR* slot)
	{
		TArray<UGroomComponent*> grooms;
		avatar.GetComponents<UGroomComponent>(grooms);
		for (UGroomComponent* candidate : grooms)
		{
			if (candidate->GetName() == slot)
			{
				return candidate;
			}
		}

		UGroomComponent* groom = NewObject<UGroomComponent>(&avatar, FName(slot));
		groom->SetupAttachment(&face);
		groom->RegisterComponent();
		return groom;
	}

	void wearLoadedGroom(UGroomComponent& groom, UGroomAsset& groomAsset, UGroomBindingAsset& binding)
	{
		// Reassigning rebuilds the groom, and both are set in one call because two calls rebuild twice, the first time on the wrong head.
		if ((groom.GroomAsset != &groomAsset) || (groom.BindingAsset != &binding))
		{
			groom.SetGroomAsset(&groomAsset, &binding, true);
		}
	}

	/** Fetched rather than loaded where this stands: a first groom cost a third of a second on the game thread, inside the screen's cross fade. */
	void wearGroom(UGroomComponent& groom, const FGroom& chosen, const FString& characterName)
	{
		const FString groomPath = FString::Printf(TEXT("%s/%s/%s/%s.%s"), groomFolder, chosen.category, chosen.name, chosen.name, chosen.name);
		const FString bindingPath = FString::Printf(TEXT("%s/%s/%s_Binding.%s_Binding"), bindingFolder, *characterName, chosen.name, chosen.name);

		UGroomAsset* groomAsset = FindObject<UGroomAsset>(nullptr, *groomPath);
		UGroomBindingAsset* binding = FindObject<UGroomBindingAsset>(nullptr, *bindingPath);
		if ((groomAsset != nullptr) && (binding != nullptr))
		{
			wearLoadedGroom(groom, *groomAsset, *binding);
			return;
		}

		for (TMap<TWeakObjectPtr<UGroomComponent>, FString>::TIterator entry = wantedGrooms.CreateIterator(); entry; ++entry)
		{
			if (!entry.Key().IsValid())
			{
				entry.RemoveCurrent();
			}
		}

		const TWeakObjectPtr<UGroomComponent> weakGroom(&groom);
		wantedGrooms.Add(weakGroom, groomPath);

		TArray<FSoftObjectPath> paths = { FSoftObjectPath(groomPath), FSoftObjectPath(bindingPath) };
		UAssetManager::GetStreamableManager().RequestAsyncLoad(paths, FStreamableDelegate::CreateLambda([weakGroom, groomPath, bindingPath]()
		{
			UGroomComponent* component = weakGroom.Get();
			const FString* wanted = wantedGrooms.Find(weakGroom);
			if ((component == nullptr) || (wanted == nullptr) || (*wanted != groomPath))
			{
				return;
			}

			UGroomAsset* loadedGroom = FindObject<UGroomAsset>(nullptr, *groomPath);
			UGroomBindingAsset* loadedBinding = FindObject<UGroomBindingAsset>(nullptr, *bindingPath);
			if ((loadedGroom == nullptr) || (loadedBinding == nullptr))
			{
				UE_LOG(LogTemp, Warning, TEXT("The appearance applier is missing %s, so that groom stays off."), (loadedGroom == nullptr) ? *groomPath : *bindingPath);
				return;
			}

			wearLoadedGroom(*component, *loadedGroom, *loadedBinding);
		}));
	}

	/** Puts the chosen groom on a slot, or clears it when the choice is out of range, so a reused component never keeps the last character's hair. */
	void applyGroom(AActor& avatar, USkeletalMeshComponent& face, const TCHAR* slot, TConstArrayView<FGroom> offered, int32 chosen, const FString& characterName)
	{
		UGroomComponent* groom = findOrCreateGroom(avatar, face, slot);
		if (!offered.IsValidIndex(chosen))
		{
			if (groom->GroomAsset != nullptr)
			{
				groom->SetGroomAsset(nullptr);
			}

			return;
		}

		wearGroom(*groom, offered[chosen], characterName);
	}

	/** Every material slot of a groom gets an instance of its own material, since the component may hand back a substituted default that reads no hair parameter. */
	void tintGroom(UGroomComponent& groom, const FHairColour& colour)
	{
		if (groom.GroomAsset == nullptr)
		{
			return;
		}

		const TArray<FHairGroupsMaterial>& slots = groom.GroomAsset->GetHairGroupsMaterials();
		for (int32 index = 0; index < slots.Num(); index++)
		{
			UMaterialInterface* parent = slots[index].Material;
			if (parent == nullptr)
			{
				continue;
			}

			UMaterialInstanceDynamic* dynamic = Cast<UMaterialInstanceDynamic>(groom.GetMaterial(index));
			if ((dynamic == nullptr) || (dynamic->Parent != parent))
			{
				dynamic = UMaterialInstanceDynamic::Create(parent, &groom);
				groom.SetMaterial(index, dynamic);
			}

			dynamic->SetScalarParameterValue(hairMelaninParameter, colour.melanin);
			dynamic->SetScalarParameterValue(hairRednessParameter, colour.redness);
		}
	}

	void applyHairColour(AActor& avatar, const Network::FAppearance& appearance)
	{
		const FHairColour& colour = hairColours[(appearance.hairColor < AppearanceChoices::hairColors) ? appearance.hairColor : 0];

		TArray<UGroomComponent*> grooms;
		avatar.GetComponents<UGroomComponent>(grooms);
		for (UGroomComponent* groom : grooms)
		{
			bool coloured = true;
			for (const TCHAR* const excluded : uncolouredGrooms)
			{
				coloured = coloured && (groom->GetName() != excluded);
			}

			if (coloured)
			{
				tintGroom(*groom, colour);
			}
		}
	}

	/** Shifts a baked skin slot rather than replacing it, since the baked material carries this character's own skin textures. */
	void tintSkinSlot(USkeletalMeshComponent& mesh, const FName& slot, float tone)
	{
		const int32 index = mesh.GetMaterialIndex(slot);
		if (index == INDEX_NONE)
		{
			return;
		}

		UMaterialInstanceDynamic* dynamic = Cast<UMaterialInstanceDynamic>(mesh.GetMaterial(index));
		if (dynamic == nullptr)
		{
			dynamic = mesh.CreateAndSetMaterialInstanceDynamic(index);
		}

		if (dynamic != nullptr)
		{
			dynamic->SetScalarParameterValue(skinValueParameter, tone);
		}
	}

	void applySkin(AActor& avatar, USkeletalMeshComponent& face, const Network::FAppearance& appearance)
	{
		const float tone = skinTones[(appearance.skinTone < AppearanceChoices::skinTones) ? appearance.skinTone : 0];
		for (const FName& slot : faceSkinSlots)
		{
			tintSkinSlot(face, slot, tone);
		}

		// The body is a separate mesh with its own material, so a face tinted alone would be a mask.
		USkeletalMeshComponent* body = findMesh(avatar, TEXT("Body"));
		if (body != nullptr)
		{
			tintSkinSlot(*body, bodySkinSlot, tone);
		}
	}

	void applyEyes(USkeletalMeshComponent& face, const Network::FAppearance& appearance)
	{
		const FEyeColour& colour = eyeColours[(appearance.eyeColor < AppearanceChoices::eyeColors) ? appearance.eyeColor : 0];
		UMaterialInterface* parametric = LoadObject<UMaterialInterface>(nullptr, parametricEyePath);
		if (parametric == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("The appearance applier could not load %s, so the eyes keep their built colour."), parametricEyePath);
			return;
		}

		for (const FName& slot : { leftEyeSlot, rightEyeSlot })
		{
			const int32 index = face.GetMaterialIndex(slot);
			if (index == INDEX_NONE)
			{
				UE_LOG(LogTemp, Warning, TEXT("The appearance applier found no eye slot %s, so the eyes keep their built colour."), *slot.ToString());
				continue;
			}

			UMaterialInstanceDynamic* dynamic = Cast<UMaterialInstanceDynamic>(face.GetMaterial(index));
			if ((dynamic == nullptr) || (dynamic->Parent != parametric))
			{
				dynamic = UMaterialInstanceDynamic::Create(parametric, &face);
				face.SetMaterial(index, dynamic);
			}

			// Primary and secondary together, or the iris blends two of the six colours at once.
			dynamic->SetScalarParameterValue(irisPrimaryHueParameter, colour.hue);
			dynamic->SetScalarParameterValue(irisPrimaryValueParameter, colour.value);
			dynamic->SetScalarParameterValue(irisSecondaryHueParameter, colour.hue);
			dynamic->SetScalarParameterValue(irisSecondaryValueParameter, colour.value);
			dynamic->SetScalarParameterValue(irisSaturationParameter, colour.saturation);
		}
	}

	/** A preset arrives in a default garment on its own component. This screen makes a person rather than dresses one, so it is hidden. */
	void undress(AActor& avatar)
	{
		TArray<USkeletalMeshComponent*> meshes;
		avatar.GetComponents<USkeletalMeshComponent>(meshes);
		for (USkeletalMeshComponent* mesh : meshes)
		{
			if ((mesh->GetName() != TEXT("Body")) && (mesh->GetName() != TEXT("Face")))
			{
				mesh->SetVisibility(false);
			}
		}
	}
}

namespace Appearance
{
	void animate(AActor& body, EMotion motion)
	{
		// The Universal Animation Library clips, baked onto the MetaHuman skeleton with MetaHuman hands and shoulders.
		static const TCHAR* const clips[] =
		{
			TEXT("/Game/Characters/Animations/UAL/Idle_Selection_Loop.Idle_Selection_Loop"),
			TEXT("/Game/Characters/Animations/UAL/Walk_Loop.Walk_Loop"),
			TEXT("/Game/Characters/Animations/UAL/Jog_Fwd_Loop.Jog_Fwd_Loop"),
			TEXT("/Game/Characters/Animations/UAL/Jump_Loop.Jump_Loop")
		};

		USkeletalMeshComponent* mesh = findMesh(body, TEXT("Body"));
		UAnimationAsset* clip = LoadObject<UAnimationAsset>(nullptr, clips[static_cast<uint8>(motion)]);
		if ((mesh == nullptr) || (clip == nullptr))
		{
			return;
		}

		const UAnimSingleNodeInstance* player = mesh->GetSingleNodeInstance();
		if ((mesh->GetAnimationMode() == EAnimationMode::AnimationSingleNode) && (player != nullptr) && (player->GetAnimationAsset() == clip))
		{
			return;
		}

		mesh->PlayAnimation(clip, true);
	}

	void apply(AActor& avatar, const Network::FAppearance& appearance)
	{
		USkeletalMeshComponent* face = findMesh(avatar, TEXT("Face"));
		const FString characterName = characterNameFor(appearance);
		if ((face == nullptr) || characterName.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("The appearance applier found no MetaHuman face on %s, so nothing was applied."), *avatar.GetName());
			return;
		}

		undress(avatar);
		applySkin(avatar, *face, appearance);
		applyEyes(*face, appearance);
		applyGroom(avatar, *face, TEXT("Hair"), hairStyles, appearance.hairStyle, characterName);
		applyGroom(avatar, *face, TEXT("Eyebrows"), eyebrowStyles, appearance.eyebrows, characterName);

		// Zero is bare, so the byte is one ahead of the list, and only men wear either, whatever a byte says.
		applyGroom(avatar, *face, TEXT("Beard"), beards, appearance.male ? (appearance.beard - 1) : -1, characterName);
		applyGroom(avatar, *face, TEXT("Mustache"), mustaches, appearance.male ? (appearance.mustache - 1) : -1, characterName);

		// After the grooms, since a groom just swapped brings its own materials with it.
		applyHairColour(avatar, appearance);
	}

	void warm()
	{
		TArray<FSoftObjectPath> paths = { FSoftObjectPath(parametricEyePath) };
		for (const TCHAR* const baseName : { femaleBaseName, maleBaseName })
		{
			if (FPackageName::DoesPackageExist(characterPackage(baseName)))
			{
				paths.Add(FSoftObjectPath(bodyClassPath(baseName)));
			}
		}

		warmed = UAssetManager::GetStreamableManager().RequestAsyncLoad(paths);
	}

	void requestBodyClass(const Network::FAppearance& appearance, TFunction<void(UClass*)> whenReady)
	{
		const FString characterName = characterNameFor(appearance);
		if (characterName.IsEmpty())
		{
			whenReady(nullptr);
			return;
		}

		const FString path = bodyClassPath(characterName);
		if (UClass* loaded = FindObject<UClass>(nullptr, *path))
		{
			whenReady(loaded);
			return;
		}

		// Keyed by path so two screens asking for the same character share the one fetch.
		pendingBodies.FindOrAdd(path) = UAssetManager::GetStreamableManager().RequestAsyncLoad(FSoftObjectPath(path), FStreamableDelegate::CreateLambda([path, whenReady]()
		{
			pendingBodies.Remove(path);
			whenReady(FindObject<UClass>(nullptr, *path));
		}));
	}

	void attachTo(AActor& host, const Network::FAppearance& appearance, const FVector& feetOffset, TFunction<void(AActor*)> whenAttached)
	{
		// A MetaHuman's mesh sits a quarter turn off its actor's yaw, the same offset the front end's spots carry.
		static constexpr float bodyYawOffset = -90.0f;

		const TWeakObjectPtr<AActor> weakHost(&host);
		requestBodyClass(appearance, [weakHost, appearance, feetOffset, whenAttached](UClass* bodyClass)
		{
			AActor* hostActor = weakHost.Get();
			if (hostActor == nullptr)
			{
				return;
			}

			UWorld* world = hostActor->GetWorld();
			if ((bodyClass == nullptr) || (world == nullptr))
			{
				whenAttached(nullptr);
				return;
			}

			FActorSpawnParameters parameters;
			parameters.Owner = hostActor;
			parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AActor* body = world->SpawnActor<AActor>(bodyClass, hostActor->GetActorTransform(), parameters);
			if (body == nullptr)
			{
				whenAttached(nullptr);
				return;
			}

			body->SetActorEnableCollision(false);
			body->AttachToActor(hostActor, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
			body->SetActorRelativeLocation(feetOffset);
			body->SetActorRelativeRotation(FRotator(0.0f, bodyYawOffset, 0.0f));
			apply(*body, appearance);
			whenAttached(body);
		});
	}
}
