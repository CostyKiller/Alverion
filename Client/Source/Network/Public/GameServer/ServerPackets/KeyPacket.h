#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The key, and whether the server encrypts at all, which L2J leaves off by default and announces in the int after the key. */
inline bool readKeyPacket(FReadBuffer& buffer, TArray<uint8>& key, bool& encrypted)
{
	uint8 result = 0;
	int32 encryption = 0;
	buffer.readUInt8(result);
	buffer.readBytes(key, 8);
	buffer.readInt32(encryption);

	encrypted = (encryption != 0);
	return !buffer.hasFailed() && (result == 1);
}
}
}
