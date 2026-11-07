#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The order is the server's: play key two before play key one, then the login pair in order. */
inline void writeAuthLogin(FWriteBuffer& buffer, const FString& account, int32 playOk1, int32 playOk2, int32 loginOk1, int32 loginOk2)
{
	buffer.writeUInt8(Opcode::authLogin);
	buffer.writeString(account.ToLower());
	buffer.writeInt32(playOk2);
	buffer.writeInt32(playOk1);
	buffer.writeInt32(loginOk1);
	buffer.writeInt32(loginOk2);
}
}
}
