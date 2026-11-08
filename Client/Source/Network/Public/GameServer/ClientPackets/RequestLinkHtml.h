#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** Asks for another dialog page by its file name, as a link in the dialog on screen names it. The server only serves pages the dialog on screen links to. */
inline bool writeRequestLinkHtml(FWriteBuffer& buffer, const FString& file)
{
	buffer.writeUInt8(Opcode::requestLinkHtml);
	return buffer.writeString(file);
}
}
}
