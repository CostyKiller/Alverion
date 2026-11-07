#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/**
 * A line of chat from a player, or a townsperson speaking.
 * Lines told by number rather than text, which only townspeople use, are reported as unreadable and left out.
 */
inline bool readCreatureSay(FReadBuffer& buffer, FChatMessage& message)
{
	int32 senderObjectId = 0;
	int32 messageId = 0;

	buffer.readInt32(senderObjectId);
	buffer.readInt32(message.channel);
	buffer.readString(message.senderName);
	buffer.readInt32(messageId);

	// No number means the text follows; a number means a line from the client's own table, which this client does not have.
	if (messageId != -1)
	{
		return false;
	}

	buffer.readString(message.text);
	if (buffer.hasFailed())
	{
		return false;
	}

	message.senderObjectId = static_cast<uint32>(senderObjectId);
	return true;
}
}
}
