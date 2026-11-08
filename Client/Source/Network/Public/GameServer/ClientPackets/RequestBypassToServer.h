#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Sends the command of a link or button in a dialog, exactly as the dialog's text carries it. The server only accepts commands that the dialog on screen offered. */
inline bool writeRequestBypassToServer(FWriteBuffer& buffer, const FString& command)
{
	buffer.writeUInt8(Opcode::requestBypassToServer);
	return buffer.writeString(command);
}
}
}
