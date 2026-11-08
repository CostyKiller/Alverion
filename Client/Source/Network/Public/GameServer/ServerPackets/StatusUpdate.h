#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The StatusUpdate attribute ids this client follows, as StatusUpdate.java numbers them. */
namespace StatusAttribute
{
	inline constexpr int32 level = 0x01;
	inline constexpr int32 currentHp = 0x09;
	inline constexpr int32 maxHp = 0x0A;
	inline constexpr int32 currentMp = 0x0B;
	inline constexpr int32 maxMp = 0x0C;
}

/** The figures of one creature that changed: its object id, a count, then that many pairs of attribute id and value. */
inline bool readStatusUpdate(FReadBuffer& buffer, FStatusUpdate& update)
{
	int32 objectId = 0;
	int32 count = 0;
	buffer.readInt32(objectId);
	buffer.readInt32(count);

	update.attributes.Reset();
	for (int32 index = 0; (index < count) && !buffer.hasFailed(); index++)
	{
		int32 id = 0;
		int32 value = 0;
		buffer.readInt32(id);
		buffer.readInt32(value);
		update.attributes.Emplace(id, value);
	}

	if (buffer.hasFailed())
	{
		return false;
	}

	update.objectId = static_cast<uint32>(objectId);
	return true;
}

/** Writes the figures a StatusUpdate carries into the character's UserInfo, leaving the attributes this client does not follow alone. */
inline void applyStatusUpdate(const FStatusUpdate& update, FUserInfo& info)
{
	for (const TPair<int32, int32>& attribute : update.attributes)
	{
		switch (attribute.Key)
		{
			case StatusAttribute::level: info.level = attribute.Value; break;
			case StatusAttribute::currentHp: info.currentHp = attribute.Value; break;
			case StatusAttribute::maxHp: info.maxHp = attribute.Value; break;
			case StatusAttribute::currentMp: info.currentMp = attribute.Value; break;
			case StatusAttribute::maxMp: info.maxMp = attribute.Value; break;
			default: break;
		}
	}
}
}
}
