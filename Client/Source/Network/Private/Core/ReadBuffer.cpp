#include "Core/ReadBuffer.h"

namespace Network
{
FReadBuffer::FReadBuffer(TArrayView<const uint8> bytes)
	: _bytes(bytes)
{
}

const uint8* FReadBuffer::take(int32 count)
{
	if (_failed || (count < 0) || (count > remaining()))
	{
		_failed = true;
		return nullptr;
	}

	const uint8* start = _bytes.GetData() + _position;
	_position += count;
	return start;
}

bool FReadBuffer::readUInt8(uint8& value)
{
	const uint8* bytes = take(1);
	if (bytes == nullptr)
	{
		return false;
	}

	value = bytes[0];
	return true;
}

bool FReadBuffer::readUInt16(uint16& value)
{
	const uint8* bytes = take(2);
	if (bytes == nullptr)
	{
		return false;
	}

	value = static_cast<uint16>(bytes[0] | (bytes[1] << 8));
	return true;
}

bool FReadBuffer::readInt16(int16& value)
{
	uint16 raw = 0;
	const bool read = readUInt16(raw);
	value = static_cast<int16>(raw);
	return read;
}

bool FReadBuffer::readInt32(int32& value)
{
	const uint8* bytes = take(4);
	if (bytes == nullptr)
	{
		return false;
	}

	const uint32 raw = static_cast<uint32>(bytes[0]) | (static_cast<uint32>(bytes[1]) << 8) | (static_cast<uint32>(bytes[2]) << 16) | (static_cast<uint32>(bytes[3]) << 24);
	value = static_cast<int32>(raw);
	return true;
}

bool FReadBuffer::readInt64(int64& value)
{
	int32 low = 0;
	int32 high = 0;
	readInt32(low);
	readInt32(high);
	value = static_cast<int64>((static_cast<uint64>(static_cast<uint32>(high)) << 32) | static_cast<uint32>(low));
	return !_failed;
}

bool FReadBuffer::readDouble(double& value)
{
	int64 raw = 0;
	if (!readInt64(raw))
	{
		return false;
	}

	FMemory::Memcpy(&value, &raw, sizeof(value));
	return true;
}

bool FReadBuffer::readString(FString& value)
{
	value.Reset();

	while (true)
	{
		uint16 unit = 0;
		if (!readUInt16(unit))
		{
			return false;
		}

		if (unit == 0)
		{
			return true;
		}

		value.AppendChar(static_cast<TCHAR>(unit));
	}
}

bool FReadBuffer::readBytes(TArray<uint8>& value, int32 count)
{
	const uint8* bytes = take(count);
	if (bytes == nullptr)
	{
		return false;
	}

	value.Reset(count);
	value.Append(bytes, count);
	return true;
}

bool FReadBuffer::skip(int32 count)
{
	return take(count) != nullptr;
}

bool FReadBuffer::hasFailed() const
{
	return _failed;
}

int32 FReadBuffer::remaining() const
{
	return _bytes.Num() - _position;
}

TArrayView<const uint8> FReadBuffer::rest() const
{
	return TArrayView<const uint8>(_bytes.GetData() + _position, remaining());
}
}
