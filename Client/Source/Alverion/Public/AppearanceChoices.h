#pragma once

#include "Appearance.h"
#include "CoreMinimal.h"

/**
 * How many of each look the create screen offers. The body that wears them reads the same numbers, so a choice can never point past what exists.
 * Provisional until the MetaHuman parts are in, at which point these become the lengths of those part lists.
 */
namespace AppearanceChoices
{
inline constexpr uint8 heads = 4;
inline constexpr uint8 skinTones = 6;
inline constexpr uint8 eyeColors = 6;
inline constexpr uint8 hairStyles = 12;
inline constexpr uint8 hairColors = 5;
inline constexpr uint8 eyebrows = 4;
inline constexpr uint8 beards = 8;
inline constexpr uint8 mustaches = 8;
}
