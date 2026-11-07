#include "Session.h"

#include "FrameStream.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlverionSession, Display, All);

namespace Network
{
FSession::~FSession()
{
	disconnect();
}

bool FSession::connect(TUniquePtr<ITransport> transport, const FString& host, int32 port, TUniquePtr<ICipher> cipher)
{
	disconnect();
	_lastError.Reset();

	if (!transport.IsValid() || !cipher.IsValid())
	{
		fail(TEXT("A link needs both a transport and a cipher."));
		return false;
	}

	_cipher = MoveTemp(cipher);
	_state = ESessionState::Connecting;

	_thread = MakeUnique<FSocketThread>(MoveTemp(transport), host, port);
	_thread->start();
	return true;
}

void FSession::disconnect()
{
	if (_thread.IsValid())
	{
		_thread->stop();
		_thread.Reset();
	}

	_cipher.Reset();
	_state = ESessionState::Disconnected;
}

ESessionState FSession::state() const
{
	return _state;
}

ICipher* FSession::cipher() const
{
	return _cipher.Get();
}

void FSession::setHandler(FPacketHandler handler)
{
	_handler = handler;
}

int32 FSession::pump(int32 budget)
{
	if (!_thread.IsValid())
	{
		return 0;
	}

	// Read before draining and acted on after: the login server sends PlayOk and hangs up at once, and that last packet must not be lost.
	const bool failed = _thread->hasFailed();
	const bool closedByPeer = _thread->hasClosedByPeer();

	if ((_state == ESessionState::Connecting) && _thread->isConnected())
	{
		_state = ESessionState::Connected;
	}

	int32 dispatched = 0;
	bool drained = false;

	// The thread is checked every pass, since a handler is free to disconnect.
	while ((dispatched < budget) && _thread.IsValid())
	{
		TArray<uint8> body;
		if (!_thread->popInbound(body))
		{
			drained = true;
			break;
		}

		if (!_cipher->decrypt(body) || body.IsEmpty())
		{
			fail(TEXT("A packet failed to decrypt."));
			disconnect();
			return dispatched;
		}

		uint32 opcode = body[0];
		int32 bodyStart = 1;

		if (opcode == extendedOpcodePrefix)
		{
			if (body.Num() < 3)
			{
				fail(TEXT("An extended packet ended before its second opcode."));
				disconnect();
				return dispatched;
			}

			opcode = extendedOpcode(static_cast<uint16>(body[1] | (body[2] << 8)));
			bodyStart = 3;
		}

		_handler.ExecuteIfBound(opcode, TArrayView<const uint8>(body.GetData() + bodyStart, body.Num() - bodyStart));
		dispatched++;
	}

	if (drained && _thread.IsValid())
	{
		if (failed)
		{
			fail(TEXT("The connection failed."));
			disconnect();
		}
		else if (closedByPeer)
		{
			disconnect();
		}
	}

	return dispatched;
}

void FSession::send(TArrayView<const uint8> body)
{
	if (!_thread.IsValid() || body.IsEmpty())
	{
		return;
	}

	TArray<uint8> encrypted(body.GetData(), body.Num());
	_cipher->encrypt(encrypted);

	const int32 totalLength = encrypted.Num() + Frame::headerSize;
	if (totalLength > Frame::maxSize)
	{
		fail(TEXT("A packet was too large for a frame."));
		return;
	}

	TArray<uint8> frame;
	frame.Reserve(totalLength);
	frame.Add(static_cast<uint8>(totalLength));
	frame.Add(static_cast<uint8>(totalLength >> 8));
	frame.Append(encrypted);

	_thread->pushOutbound(MoveTemp(frame));
}

bool FSession::hasFailed() const
{
	return !_lastError.IsEmpty();
}

const FString& FSession::lastError() const
{
	return _lastError;
}

void FSession::fail(const FString& reason)
{
	UE_LOG(LogAlverionSession, Warning, TEXT("%s"), *reason);

	if (_lastError.IsEmpty())
	{
		_lastError = reason;
	}
}
}
