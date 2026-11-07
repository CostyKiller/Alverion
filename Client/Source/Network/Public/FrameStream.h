#pragma once

#include "CoreMinimal.h"

namespace Network
{

// The protocol's framing, the same on the login link and the game link.
// A little-endian uint16 that counts itself as well as the body, then the body, which is still encrypted here.
// Decryption is stateful and runs on the game thread in arrival order, so this only cuts frames apart.
namespace Frame
{
	inline constexpr int32 headerSize = 2;
	inline constexpr int32 maxSize = 65535;

	enum class EParseResult : uint8
	{
		Complete,
		Incomplete,
		Violation
	};
}

class NETWORK_API FFrameStream
{
public:
	void append(TArrayView<const uint8> bytes);

	Frame::EParseResult pop(TArray<uint8>& body);

	bool isBroken() const;
	int32 pending() const;
	void reset();

private:
	TArray<uint8> _bytes;
	int32 _consumed = 0;
	bool _broken = false;
};

}
