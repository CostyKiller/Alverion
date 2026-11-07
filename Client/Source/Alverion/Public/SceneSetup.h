#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;
class UWorld;

/** Scenes built in code on the engine's empty Entry map, since the project keeps no maps of its own yet. */
namespace SceneSetup
{
/** Sun, sky, sky light and haze. */
void buildDaylight(UWorld& world);

/** A coloured cylinder of a person's size, standing in for a body until MetaHumans wear the appearance block. */
void paintPlaceholderBody(UStaticMeshComponent& body, const FLinearColor& color);
}
