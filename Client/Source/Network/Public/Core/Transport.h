#pragma once

#include "CoreMinimal.h"

class FSocket;

namespace Network
{
enum class EReceiveResult : uint8
{
	Healthy,
	ClosedByPeer,
	Failed
};

/** A byte pipe. Everything protocol about the bytes happens above it. Called from the socket thread only. */
class NETWORK_API ITransport
{
public:
	virtual ~ITransport() = default;

	virtual bool connect(const FString& host, int32 port) = 0;

	/** Appends whatever arrived within a few milliseconds, possibly nothing. */
	virtual EReceiveResult receive(TArray<uint8>& into) = 0;

	virtual bool send(TArrayView<const uint8> bytes) = 0;
	virtual void close() = 0;
};

class NETWORK_API FTcpTransport final : public ITransport
{
public:
	virtual ~FTcpTransport() override;

	virtual bool connect(const FString& host, int32 port) override;
	virtual EReceiveResult receive(TArray<uint8>& into) override;
	virtual bool send(TArrayView<const uint8> bytes) override;
	virtual void close() override;

private:
	FSocket* _socket = nullptr;
};
}
