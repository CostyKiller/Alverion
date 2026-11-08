#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** A dialog page from a townsperson or from the server itself. Which townsperson is not needed to show it, so only the text is kept with the id. */
inline bool readNpcHtmlMessage(FReadBuffer& buffer, FNpcHtml& message)
{
	int32 npcObjectId = 0;
	int32 itemId = 0;

	buffer.readInt32(npcObjectId);
	buffer.readString(message.html);
	buffer.readInt32(itemId);
	if (buffer.hasFailed())
	{
		return false;
	}

	message.npcObjectId = static_cast<uint32>(npcObjectId);
	message.itemId = itemId;
	return true;
}
}
}
