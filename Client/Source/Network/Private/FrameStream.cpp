#include "FrameStream.h"

namespace Network
{

namespace
{
	// Compacting on every pop would copy the whole tail each time,
	// so it waits until the dead prefix is worth reclaiming.
	constexpr int32 compactThreshold = 4096;
}

void FFrameStream::append(TArrayView<const uint8> bytes)
{
	if (_broken || (bytes.Num() <= 0))
	{
		return;
	}

	_bytes.Append(bytes.GetData(), bytes.Num());
}

Frame::EParseResult FFrameStream::pop(TArray<uint8>& body)
{
	if (_broken)
	{
		return Frame::EParseResult::Violation;
	}

	const int32 unread = _bytes.Num() - _consumed;
	if (unread < Frame::headerSize)
	{
		return Frame::EParseResult::Incomplete;
	}

	const uint8* start = _bytes.GetData() + _consumed;
	const int32 totalLength = static_cast<int32>(start[0]) | (static_cast<int32>(start[1]) << 8);

	// A length too short to hold its own header can never be satisfied, and waiting for it would stall the link forever.
	if (totalLength <= Frame::headerSize)
	{
		_broken = true;
		return Frame::EParseResult::Violation;
	}

	if (unread < totalLength)
	{
		return Frame::EParseResult::Incomplete;
	}

	const int32 bodyLength = totalLength - Frame::headerSize;
	body.Reset(bodyLength);
	body.Append(start + Frame::headerSize, bodyLength);

	_consumed += totalLength;

	if (_consumed >= _bytes.Num())
	{
		_bytes.Reset();
		_consumed = 0;
	}
	else if (_consumed >= compactThreshold)
	{
		_bytes.RemoveAt(0, _consumed, EAllowShrinking::No);
		_consumed = 0;
	}

	return Frame::EParseResult::Complete;
}

bool FFrameStream::isBroken() const
{
	return _broken;
}

int32 FFrameStream::pending() const
{
	return _bytes.Num() - _consumed;
}

void FFrameStream::reset()
{
	_bytes.Reset();
	_consumed = 0;
	_broken = false;
}

}
