#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/** The creature kind trailer, read from the end of a packet that is longer than it: the magic, then one byte for the kind. */
inline ECreatureKind readCreatureKind(TArrayView<const uint8> body)
{
	static constexpr int32 trailerBytes = 5;

	FReadBuffer trailer(TArrayView<const uint8>(body.GetData() + body.Num() - trailerBytes, trailerBytes));
	int32 magic = 0;
	uint8 kind = 0;
	trailer.readInt32(magic);
	trailer.readUInt8(kind);

	return (magic == creatureKindMagic) ? static_cast<ECreatureKind>(kind) : ECreatureKind::Normal;
}
}
}
