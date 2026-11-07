#pragma once

#include "CoreMinimal.h"

class SWidget;

/**
 * The full screen cards shown while a map loads, shared by the boot, intro and loading maps and by the movie player.
 * The movie player draws during a map load on its own thread, the only thing that can while the game thread is blocked, but its renderer
 * crashed about half the time on this engine, so it is off unless a run asks for it with -loadingscreen.
 * The boot map is black and says nothing; the intro map wears the studio's mark and is the front end's wait as much as its greeting;
 * a load anywhere else says so in words.
 */
namespace client
{
	ALVERION_API TSharedRef<SWidget> makeBootScreen();
	ALVERION_API TSharedRef<SWidget> makeIntroScreen();
	ALVERION_API TSharedRef<SWidget> makeLoadingScreen();
}
