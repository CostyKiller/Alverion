#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
inline bool readCharacterSelected(FReadBuffer& buffer, FCharacterSelected& selected)
{
	int32 objectId = 0;
	int32 female = 0;
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;
	double hp = 0.0;

	buffer.readString(selected.name);
	buffer.readInt32(objectId);
	buffer.readString(selected.title);

	// Session id, clan, an unknown.
	buffer.skip(12);

	buffer.readInt32(female);
	buffer.readInt32(selected.race);
	buffer.readInt32(selected.classId);

	// Active.
	buffer.skip(4);

	buffer.readInt32(x);
	buffer.readInt32(y);
	buffer.readInt32(z);
	buffer.readDouble(hp);

	// MP, SP, experience.
	buffer.skip(8 + 4 + 8);

	buffer.readInt32(selected.level);

	if (buffer.hasFailed())
	{
		return false;
	}

	selected.objectId = static_cast<uint32>(objectId);
	selected.female = (female != 0);
	selected.location = toLocation(x, y, z);
	selected.currentHp = FMath::RoundToInt32(hp);
	return true;
}
}
}
