#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
inline void writeProtocolVersion(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::protocolVersion);
	buffer.writeInt32(protocolRevision);
}
}
}
