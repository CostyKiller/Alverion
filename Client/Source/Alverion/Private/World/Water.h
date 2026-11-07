#pragma once

#include "CoreMinimal.h"

/**
 * The sea: one surface at height zero, where the server's sea level lands.
 * A character stands while the water over the ground leaves its head out, and swims where it would not.
 */
namespace Water
{
	inline constexpr double surfaceCm = 0.0;

	/** Water deeper than this over the ground reaches a standing character's chin, so it swims. */
	inline constexpr double swimDepthCm = 140.0;

	/** A swimmer stands again only in water shallower than this, so the two never flicker at the edge. */
	inline constexpr double wadeDepthCm = 120.0;

	/** As high as a swimmer's feet origin floats. The swim clips carry the shoulders there, so this keeps arms and legs under with the water at the neck. */
	inline constexpr double swimmerFeetCm = surfaceCm - 20.0;

	/** Whether ground this high leaves water too deep to stand in, for a character that swims already or not. */
	inline bool tooDeep(double groundCm, bool swimming)
	{
		return (surfaceCm - groundCm) > (swimming ? wadeDepthCm : swimDepthCm);
	}
}
