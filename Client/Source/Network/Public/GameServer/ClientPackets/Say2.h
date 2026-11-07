#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The server turns away anything longer than this, so the client never sends it. */
inline constexpr int32 chatLineMaxLength = 105;

/** One line of chat on a channel. Whispers carry a name after the channel, and this client sends none yet. */
inline void writeSay2(FWriteBuffer& buffer, const FString& text, EChatChannel channel)
{
	buffer.writeUInt8(Opcode::say2);
	buffer.writeString(text.Left(chatLineMaxLength));
	buffer.writeInt32(static_cast<int32>(channel));
}
}
}
