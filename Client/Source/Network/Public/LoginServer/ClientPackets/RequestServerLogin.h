#pragma once

#include "LoginServer/LoginServerProtocol.h"

namespace Network
{
namespace LoginServer
{
inline void writeRequestServerLogin(FWriteBuffer& buffer, int32 loginOk1, int32 loginOk2, uint8 serverId)
{
	buffer.writeUInt8(Opcode::requestServerLogin);
	buffer.writeInt32(loginOk1);
	buffer.writeInt32(loginOk2);
	buffer.writeUInt8(serverId);
}
}
}
