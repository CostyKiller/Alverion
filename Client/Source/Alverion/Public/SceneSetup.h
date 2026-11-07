#pragma once

#include "CoreMinimal.h"

class UStaticMeshComponent;

/** Placeholder bodies shared by the front end preview, the player and everybody the server shows. */
namespace SceneSetup
{
/** A coloured cylinder of a person's size, standing in for a body until MetaHumans wear the appearance block. */
void paintPlaceholderBody(UStaticMeshComponent& body, const FLinearColor& color);
}
