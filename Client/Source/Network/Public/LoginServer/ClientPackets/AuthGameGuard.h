#pragma once

#include "LoginServer/LoginServerProtocol.h"

namespace Network
{
namespace LoginServer
{
inline void writeAuthGameGuard(FWriteBuffer& buffer, int32 sessionId)
{
	buffer.writeUInt8(Opcode::authGameGuard);
	buffer.writeInt32(sessionId);

	// Where a real GameGuard answer would go. The server reads and discards four words.
	buffer.writeZeros(16);
}
}
}
