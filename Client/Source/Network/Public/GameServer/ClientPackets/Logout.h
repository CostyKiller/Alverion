#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
inline void writeLogout(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::logout);
}
}
}
