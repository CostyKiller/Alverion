#pragma once

#include "Appearance.h"
#include "CoreMinimal.h"

/**
 * How many of each look the create screen offers: the lengths of the MetaHuman part lists the body will wear, so a choice can never point past what exists.
 * Heads count per sex. Beards and mustaches count none as their first choice.
 */
namespace AppearanceChoices
{
inline constexpr uint8 heads = 4;
inline constexpr uint8 skinTones = 6;
inline constexpr uint8 eyeColors = 6;
inline constexpr uint8 hairStyles = 34;
inline constexpr uint8 hairColors = 5;
inline constexpr uint8 eyebrows = 17;
inline constexpr uint8 beards = 17;
inline constexpr uint8 mustaches = 17;
}
