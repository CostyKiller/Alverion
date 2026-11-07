#pragma once

#include "CoreMinimal.h"
#include "Core/ServerCrypt.h"
#include "Core/SocketThread.h"

namespace Network
{
/** What the link itself is doing. Where the conversation over it stands belongs to the flow driving it. */
enum class ESessionState : uint8
{
	Disconnected,
	Connecting,
	Connected
};

/** The extended packets' first byte. The handler sees such a packet's opcode as this, shifted up, over the uint16 that follows. */
inline constexpr uint32 extendedOpcodePrefix = 0xFE;

inline constexpr uint32 extendedOpcode(uint16 subOpcode)
{
	return (extendedOpcodePrefix << 16) | subOpcode;
}

/** The opcode, then the decrypted body after it. */
DECLARE_DELEGATE_TwoParams(FPacketHandler, uint32, TArrayView<const uint8>);

/**
 * One TCP link to a login or game server.
 * Frames are decrypted here on the game thread, one at a time and in order, because handling one frame can rekey the next.
 */
class NETWORK_API FSession
{
public:
	~FSession();

	bool connect(TUniquePtr<ITransport> transport, const FString& host, int32 port, TUniquePtr<ICipher> cipher);
	void disconnect();

	ESessionState state() const;
	ICipher* cipher() const;

	void setHandler(FPacketHandler handler);

	/** Decrypts and dispatches up to budget packets, and returns how many it dispatched. */
	int32 pump(int32 budget);

	/** A packet body, opcode first. Encrypted and framed here. */
	void send(TArrayView<const uint8> body);

	bool hasFailed() const;
	const FString& lastError() const;

private:
	void fail(const FString& reason);

	TUniquePtr<FSocketThread> _thread;
	TUniquePtr<ICipher> _cipher;
	FPacketHandler _handler;
	ESessionState _state = ESessionState::Disconnected;
	FString _lastError;
};
}
