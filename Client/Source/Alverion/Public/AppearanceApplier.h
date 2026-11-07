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
}
