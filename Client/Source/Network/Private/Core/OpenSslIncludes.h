#pragma once

#include "CoreMinimal.h"

// One copy of the include dance, because getting it wrong is a wall of errors rather than one.
#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

// OpenSSL typedefs UI while the engine has a namespace by that name.
// Renaming the typedef is the only side that can move, since the namespace is spelled all over the engine.
#define UI OPENSSL_UI

THIRD_PARTY_INCLUDES_START
#include <openssl/blowfish.h>
#include <openssl/bn.h>
THIRD_PARTY_INCLUDES_END

#undef UI

#if PLATFORM_WINDOWS
#include "Windows/HideWindowsPlatformTypes.h"
#endif
