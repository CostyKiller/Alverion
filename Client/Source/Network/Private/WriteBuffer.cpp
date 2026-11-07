#include "WriteBuffer.h"

namespace Network
{
void FWriteBuffer::writeUInt8(uint8 value)
{
	_bytes.Add(value);
}

void FWriteBuffer::writeInt16(int16 value)
{
	const uint16 raw = static_cast<uint16>(value);
	_bytes.Add(static_cast<uint8>(raw));
	_bytes.Add(static_cast<uint8>(raw >> 8));
}

void FWriteBuffer::writeInt32(int32 value)
{
	const uint32 raw = static_cast<uint32>(value);
	for (int32 shift = 0; shift < 32; shift += 8)
	{
		_bytes.Add(static_cast<uint8>(raw >> shift));
	}
}

void FWriteBuffer::writeDouble(double value)
{
	uint64 raw = 0;
	FMemory::Memcpy(&raw, &value, sizeof(raw));
	for (int32 shift = 0; shift < 64; shift += 8)
	{
		_bytes.Add(static_cast<uint8>(raw >> shift));
	}
}

void FWriteBuffer::writeBytes(TArrayView<const uint8> value)
{
	_bytes.Append(value.GetData(), value.Num());
}

void FWriteBuffer::writeZeros(int32 count)
{
	_bytes.AddZeroed(count);
}

bool FWriteBuffer::writeString(const FString& value)
{
	// Checked by hand, because FindChar finds the terminator itself when asked for a zero.
	for (int32 index = 0; index < value.Len(); index++)
	{
		if (value[index] == TEXT('\0'))
		{
			return false;
		}
	}

	const FTCHARToUTF16 converted(*value);
	const UTF16CHAR* units = reinterpret_cast<const UTF16CHAR*>(converted.Get());
	for (int32 index = 0; index < converted.Length(); index++)
	{
		writeInt16(static_cast<int16>(units[index]));
	}

	writeInt16(0);
	return true;
}

TArrayView<const uint8> FWriteBuffer::bytes() const
{
	return _bytes;
}
}
