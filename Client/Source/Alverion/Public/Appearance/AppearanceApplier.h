#pragma once

#include "CoreMinimal.h"

class AActor;
class UClass;

namespace Network
{
	struct FAppearance;
}

/**
 * Puts an appearance onto a spawned MetaHuman, in one place, so character creation, character select and a stranger in town agree from the same bytes.
 * Each head is a whole character rather than a face on a shared body, because a MetaHuman bakes head and body textures together and they only match as a pair.
 * Grooms wear bindings baked for that character's face; hair colour, skin tone and eye colour shift the materials the character was built with.
 */
namespace Appearance
{
	/** What a body is doing, which picks the clip it plays. Characters only run, on land, and swim in deep water. */
	enum class EMotion : uint8
	{
		Idle,
		Run,
		Fall,
		SwimIdle,
		Swim
	};

	/** Plays the clip for a motion on a MetaHuman's body, looping, and leaves it alone while the motion stays the same, so a loop keeps its phase. */
	ALVERION_API void animate(AActor& body, EMotion motion);

	/** Applies what it can and leaves the rest: a missing groom or eye material is logged and skipped, since a bald character beats no character. */
	ALVERION_API void apply(AActor& avatar, const Network::FAppearance& appearance);

	/** Fetches what every character needs, so the first one shown does not stall the game thread. Harmless to call again. */
	ALVERION_API void warm();

	/**
	 * Answers with the character class an appearance wears, at once when it is in memory and later when it had to be fetched.
	 * A character is about two hundred megabytes, and loading one where it is wanted stops the game thread for a second.
	 * The answer is null when the MetaHuman assets are not installed, which a fresh clone is.
	 */
	ALVERION_API void requestBodyClass(const Network::FAppearance& appearance, TFunction<void(UClass*)> whenReady);

	/**
	 * Stands the MetaHuman an appearance describes on a host whose feet are feetOffset from its root, facing the host's forward, and dresses it.
	 * The body has no collision of its own, so it never blocks the host it rides on. The callback gets it once it stands, or null when the MetaHuman assets are missing.
	 * The host owns it from then on and destroys it with itself, since an attached actor outlives its parent otherwise.
	 */
	ALVERION_API void attachTo(AActor& host, const Network::FAppearance& appearance, const FVector& feetOffset, TFunction<void(AActor*)> whenAttached);
}
