#pragma once

#include "CoreMinimal.h"
#include "LoginServer/LoginServerPackets.h"
#include "Session.h"

namespace Network
{
/** Init, GameGuard, credentials, LoginOk, the server list, then PlayOk, which is the token the game server asks for. */
enum class ELoginServerStep : uint8
{
	Idle,
	Connecting,
	Authenticating,
	ServerListRequested,
	ServerSelect,
	SelectSent,
	TokenIssued,
	Failed
};

DECLARE_DELEGATE_OneParam(FLoginServerStepChanged, ELoginServerStep);
DECLARE_DELEGATE_OneParam(FLoginServerFailed, const FString&);

/** The login server leg. Its session must be opened with an FLoginCipher, which this flow keys when Init arrives. */
class NETWORK_API FLoginServerFlow
{
public:
	~FLoginServerFlow();

	void begin(FSession& session, const FString& account, const FString& password);
	void tick();
	bool selectServer(uint8 serverId);
	void end();

	ELoginServerStep step() const;
	const TArray<FServerListEntry>& servers() const;
	const FLoginToken& token() const;
	const FString& lastError() const;

	FLoginServerStepChanged onStepChanged;
	FLoginServerFailed onFailed;

private:
	void handlePacket(uint32 opcode, TArrayView<const uint8> body);
	void send(const FWriteBuffer& buffer);
	void setStep(ELoginServerStep next);
	void fail(const FString& reason);
	void scrubPassword();

	FSession* _session = nullptr;
	FString _password;
	TArray<uint8> _modulus;
	uint8 _selectedServer = 0;
	TArray<FServerListEntry> _servers;
	FLoginToken _token;
	FString _lastError;
	ELoginServerStep _step = ELoginServerStep::Idle;
};
}
