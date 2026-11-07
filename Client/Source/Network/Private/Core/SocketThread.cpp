#include "Core/SocketThread.h"

#include "Core/FrameStream.h"
#include "HAL/RunnableThread.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlverionSocket, Display, All);

namespace Network
{
FSocketThread::FSocketThread(TUniquePtr<ITransport> transport, const FString& host, int32 port)
	: _transport(MoveTemp(transport))
	, _host(host)
	, _port(port)
{
}

FSocketThread::~FSocketThread()
{
	stop();
}

void FSocketThread::start()
{
	if (_thread == nullptr)
	{
		_thread = FRunnableThread::Create(this, TEXT("AlverionSocket"));
	}
}

void FSocketThread::stop()
{
	_stopRequested = true;

	if (_thread != nullptr)
	{
		_thread->WaitForCompletion();
		delete _thread;
		_thread = nullptr;
	}
}

void FSocketThread::Stop()
{
	_stopRequested = true;
}

bool FSocketThread::popInbound(TArray<uint8>& body)
{
	return _inbound.Dequeue(body);
}

void FSocketThread::pushOutbound(TArray<uint8>&& frame)
{
	_outbound.Enqueue(MoveTemp(frame));
}

bool FSocketThread::isConnected() const
{
	return _connected;
}

bool FSocketThread::hasFailed() const
{
	return _failed;
}

bool FSocketThread::hasClosedByPeer() const
{
	return _closedByPeer;
}

uint32 FSocketThread::Run()
{
	if (!_transport->connect(_host, _port))
	{
		UE_LOG(LogAlverionSocket, Warning, TEXT("Could not connect to %s:%d."), *_host, _port);
		_failed = true;
		return 0;
	}

	_connected = true;

	FFrameStream stream;
	TArray<uint8> received;

	while (!_stopRequested && !_failed && !_closedByPeer)
	{
		received.Reset();
		const EReceiveResult result = _transport->receive(received);
		stream.append(received);

		TArray<uint8> body;
		Frame::EParseResult parsed = stream.pop(body);
		while (parsed == Frame::EParseResult::Complete)
		{
			_inbound.Enqueue(MoveTemp(body));
			parsed = stream.pop(body);
		}

		if (parsed == Frame::EParseResult::Violation)
		{
			UE_LOG(LogAlverionSocket, Warning, TEXT("%s:%d sent a frame that cannot be one."), *_host, _port);
			_failed = true;
		}

		if (result == EReceiveResult::ClosedByPeer)
		{
			_closedByPeer = true;
		}
		else if (result == EReceiveResult::Failed)
		{
			_failed = true;
		}

		TArray<uint8> frame;
		while (!_failed && _outbound.Dequeue(frame))
		{
			if (!_transport->send(frame))
			{
				_failed = true;
			}
		}
	}

	_transport->close();
	_connected = false;
	return 0;
}
}
