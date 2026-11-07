#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/**
 * The stock fields, then the extension's appearance after the magic, which an unpatched server never reads.
 * The six base stats the server ignores are zero, and the race it keeps comes from the class template.
 */
inline void writeCharacterCreate(FWriteBuffer& buffer, const FString& name, bool female, int32 classId, int32 hairStyle, int32 hairColor, int32 face, TArrayView<const uint8> appearance)
{
	buffer.writeUInt8(Opcode::characterCreate);
	buffer.writeString(name);
	buffer.writeInt32(static_cast<int32>(ERace::Human));
	buffer.writeInt32(female ? 1 : 0);
	buffer.writeInt32(classId);
	buffer.writeZeros(6 * 4);
	buffer.writeInt32(hairStyle);
	buffer.writeInt32(hairColor);
	buffer.writeInt32(face);

	buffer.writeInt32(appearanceMagic);
	buffer.writeUInt8(static_cast<uint8>(appearance.Num()));
	buffer.writeBytes(appearance);
}
}
}
