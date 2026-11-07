#pragma once

#include "Containers/Queue.h"
#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Core/Transport.h"

class FRunnableThread;

namespace Network
{
/**
 * Owns one link's socket on its own thread: connects, cuts the incoming stream into frames and writes what the game thread queued.
 * Frames cross in both directions still encrypted, because the ciphers carry state that only the game thread may advance.
 */
class NETWORK_API FSocketThread final : public FRunnable
{
public:
	FSocketThread(TUniquePtr<ITransport> transport, const FString& host, int32 port);
	virtual ~FSocketThread() override;

	void start();
	void stop();

	/** One encrypted frame body, its length header already removed. */
	bool popInbound(TArray<uint8>& body);

	/** One whole frame, length header included. */
	void pushOutbound(TArray<uint8>&& frame);

	bool isConnected() const;
	bool hasFailed() const;
	bool hasClosedByPeer() const;

	virtual uint32 Run() override;
	virtual void Stop() override;

private:
	TUniquePtr<ITransport> _transport;
	FString _host;
	int32 _port = 0;
	FRunnableThread* _thread = nullptr;

	TQueue<TArray<uint8>, EQueueMode::Spsc> _inbound;
	TQueue<TArray<uint8>, EQueueMode::Spsc> _outbound;

	std::atomic<bool> _stopRequested = false;
	std::atomic<bool> _connected = false;
	std::atomic<bool> _failed = false;
	std::atomic<bool> _closedByPeer = false;
};
}
