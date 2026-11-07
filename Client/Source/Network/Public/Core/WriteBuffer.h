#pragma once

#include "CoreMinimal.h"

namespace Network
{
/** Builds a packet body, opcode first: little-endian numbers and zero-terminated UTF-16 strings. */
class NETWORK_API FWriteBuffer
{
public:
	void writeUInt8(uint8 value);
	void writeInt16(int16 value);
	void writeInt32(int32 value);
	void writeDouble(double value);
	void writeBytes(TArrayView<const uint8> value);
	void writeZeros(int32 count);

	/** Writes nothing and returns false for a string holding a zero character, since the reader would stop there. */
	bool writeString(const FString& value);

	TArrayView<const uint8> bytes() const;

private:
	TArray<uint8> _bytes;
};
}
