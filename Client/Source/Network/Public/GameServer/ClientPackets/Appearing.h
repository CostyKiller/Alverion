#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Sent after every TeleportToLocation aimed at this client. Until it arrives the server holds the character as teleporting and ignores its positions. */
inline void writeAppearing(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::appearing);
}
}
}
