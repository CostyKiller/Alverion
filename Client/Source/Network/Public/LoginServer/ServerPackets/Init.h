#pragma once

#include "LoginServer/LoginServerProtocol.h"

namespace Network
{
namespace LoginServer
{
struct FInit
{
	int32 sessionId = 0;
	int32 protocol = 0;
	TArray<uint8> modulus;
	TArray<uint8> blowfishKey;

	bool read(FReadBuffer& buffer)
	{
		static constexpr int32 gameGuardBytes = 16;
		static constexpr int32 blowfishKeyBytes = 16;

		buffer.readInt32(sessionId);
		buffer.readInt32(protocol);
		buffer.readBytes(modulus, ServerCrypt::rsaBlockSize);
		buffer.skip(gameGuardBytes);
		buffer.readBytes(blowfishKey, blowfishKeyBytes);
		return !buffer.hasFailed();
	}
};
}
}
