#pragma once

#include "LoginServer/LoginServerProtocol.h"

namespace Network
{
namespace LoginServer
{
inline void writeRequestServerList(FWriteBuffer& buffer, int32 loginOk1, int32 loginOk2)
{
	buffer.writeUInt8(Opcode::requestServerList);
	buffer.writeInt32(loginOk1);
	buffer.writeInt32(loginOk2);
}
}
}
