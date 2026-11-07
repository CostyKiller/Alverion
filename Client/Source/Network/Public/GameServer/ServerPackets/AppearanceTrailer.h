#pragma once

#include "GameServer/GameServerProtocol.h"

namespace Network
{
namespace GameServer
{
/**
 * The extension's trailer, read from the end of a packet so nothing before it has to be understood:
 * count blobs of size bytes each, then size and count as int16, then the magic.
 */
inline bool readAppearanceTrailer(TArrayView<const uint8> body, TArray<TArray<uint8>>& blobs)
{
	static constexpr int32 footerBytes = 8;

	blobs.Reset();
	if (body.Num() < footerBytes)
	{
		return false;
	}

	FReadBuffer footer(TArrayView<const uint8>(body.GetData() + body.Num() - footerBytes, footerBytes));
	int16 size = 0;
	int16 count = 0;
	int32 magic = 0;
	footer.readInt16(size);
	footer.readInt16(count);
	footer.readInt32(magic);

	const int32 blobBytes = static_cast<int32>(size) * static_cast<int32>(count);
	if ((magic != appearanceMagic) || (size <= 0) || (count <= 0) || (blobBytes > (body.Num() - footerBytes)))
	{
		return false;
	}

	const uint8* start = body.GetData() + body.Num() - footerBytes - blobBytes;
	for (int32 index = 0; index < count; index++)
	{
		blobs.Emplace(start + (index * size), size);
	}

	return true;
}
}
}
