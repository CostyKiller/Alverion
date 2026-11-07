#pragma once

#include "CoreMinimal.h"

namespace Network
{
/**
 * How a character looks, beyond what the server itself keeps.
 * The server extension stores these bytes as they are and hands them back in the character list and in CharInfo, so only this client gives them meaning.
 * Fixed at 24 bytes; the spare tail is where later choices go, and the version says which layout wrote them.
 */
struct NETWORK_API FAppearance
{
	static constexpr int32 size = 24;
	static constexpr uint8 currentVersion = 1;

	uint8 version = currentVersion;
	bool male = false;
	uint8 head = 0;
	uint8 skinTone = 0;
	uint8 eyeColor = 0;
	uint8 hairStyle = 0;
	uint8 hairColor = 0;
	uint8 eyebrows = 0;
	uint8 beard = 0;
	uint8 mustache = 0;

	TArray<uint8> pack() const
	{
		TArray<uint8> bytes;
		bytes.SetNumZeroed(size);
		bytes[0] = version;
		bytes[1] = male ? 1 : 0;
		bytes[2] = head;
		bytes[3] = skinTone;
		bytes[4] = eyeColor;
		bytes[5] = hairStyle;
		bytes[6] = hairColor;
		bytes[7] = eyebrows;
		bytes[8] = beard;
		bytes[9] = mustache;
		return bytes;
	}

	/** False for anything not written by a known version, which the caller treats as no appearance at all. */
	static bool unpack(TArrayView<const uint8> bytes, FAppearance& out)
	{
		if ((bytes.Num() != size) || (bytes[0] != currentVersion))
		{
			return false;
		}

		out.version = bytes[0];
		out.male = (bytes[1] != 0);
		out.head = bytes[2];
		out.skinTone = bytes[3];
		out.eyeColor = bytes[4];
		out.hairStyle = bytes[5];
		out.hairColor = bytes[6];
		out.eyebrows = bytes[7];
		out.beard = bytes[8];
		out.mustache = bytes[9];
		return true;
	}
};
}
