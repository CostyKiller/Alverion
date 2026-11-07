#pragma once

#include "CoreMinimal.h"
#include "Core/ServerCrypt.h"
#include "Core/ReadBuffer.h"
#include "Core/WriteBuffer.h"

/**
 * The login server's packets, as L2J Mobius writes and reads them.
 * Reads stop at what this client uses. The rest of a packet, padding and checksum included, is left unread.
 */
namespace Network
{
enum class EServerStatus : uint8
{
	Offline,
	Online
};

struct NETWORK_API FServerListEntry
{
	uint8 serverId = 0;

	/** The server sends no names, so this is made from the id. */
	FString name;

	EServerStatus status = EServerStatus::Offline;

	/** Where the game server listens, as the login server reports it for this client's address. */
	FString host;
	int32 port = 0;

	uint16 currentPlayers = 0;
	uint16 maxPlayers = 0;
};

/** Everything the login server leg ends with: where the game server is, and what it asks for in AuthLogin. */
struct NETWORK_API FLoginToken
{
	FString host;
	int32 port = 0;
	FString account;
	int32 loginOk1 = 0;
	int32 loginOk2 = 0;
	int32 playOk1 = 0;
	int32 playOk2 = 0;
};

/** The LoginFail and PlayFail codes worth a name. Anything else is shown by number. */
inline FString loginFailReasonName(int32 code)
{
	switch (code)
	{
		case 0x02:
		{
			return TEXT("wrong account or password");
		}
		case 0x04:
		{
			return TEXT("access failed");
		}
		case 0x07:
		{
			return TEXT("account in use");
		}
		case 0x0F:
		{
			return TEXT("server overloaded");
		}
		case 0x10:
		{
			return TEXT("server maintenance");
		}
		default:
		{
			return FString::Printf(TEXT("code 0x%02X"), code);
		}
	}
}

namespace LoginServer
{
namespace Opcode
{
	// Server to client.
	inline constexpr uint32 init = 0x00;
	inline constexpr uint32 loginFail = 0x01;
	inline constexpr uint32 accountKicked = 0x02;
	inline constexpr uint32 loginOk = 0x03;
	inline constexpr uint32 serverList = 0x04;
	inline constexpr uint32 playFail = 0x06;
	inline constexpr uint32 playOk = 0x07;
	inline constexpr uint32 ggAuth = 0x0B;

	// Client to server.
	inline constexpr uint8 requestAuthLogin = 0x00;
	inline constexpr uint8 requestServerLogin = 0x02;
	inline constexpr uint8 requestServerList = 0x05;
	inline constexpr uint8 authGameGuard = 0x07;
}
}
}
