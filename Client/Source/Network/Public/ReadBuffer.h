#pragma once

#include "CoreMinimal.h"

namespace Network
{
/**
 * Reads a packet body: little-endian numbers and zero-terminated UTF-16 strings.
 * A read past the end fails the buffer for good, so a run of reads can be checked once at the end.
 */
class NETWORK_API FReadBuffer
{
public:
	explicit FReadBuffer(TArrayView<const uint8> bytes);

	bool readUInt8(uint8& value);
	bool readUInt16(uint16& value);
	bool readInt16(int16& value);
	bool readInt32(int32& value);
	bool readInt64(int64& value);
	bool readDouble(double& value);
	bool readString(FString& value);
	bool readBytes(TArray<uint8>& value, int32 count);
	bool skip(int32 count);

	bool hasFailed() const;
	int32 remaining() const;

	/** The unread bytes, without consuming them. */
	TArrayView<const uint8> rest() const;

private:
	const uint8* take(int32 count);

	TArrayView<const uint8> _bytes;
	int32 _position = 0;
	bool _failed = false;
};
}
