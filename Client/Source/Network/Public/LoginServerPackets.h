#pragma once

#include "CoreMinimal.h"
#include "ServerCrypt.h"
#include "ReadBuffer.h"
#include "WriteBuffer.h"

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

struct FInit
{
	int32 sessionId = 0;
	int32 protocol = 0;
	TArray<uint8> modulus;
	TArray<uint8> blowfishKey;

	bool read(FReadBuffer& buffer)
	{
		static constexpr int32 gameGuardBytes = 16;
		static constexpr int32 blowfishKeyBytes = 16;

		buffer.readInt32(sessionId);
		buffer.readInt32(protocol);
		buffer.readBytes(modulus, ServerCrypt::rsaBlockSize);
		buffer.skip(gameGuardBytes);
		buffer.readBytes(blowfishKey, blowfishKeyBytes);
		return !buffer.hasFailed();
	}
};

struct FServerList
{
	TArray<FServerListEntry> servers;

	bool read(FReadBuffer& buffer)
	{
		uint8 count = 0;
		uint8 lastServer = 0;
		buffer.readUInt8(count);
		buffer.readUInt8(lastServer);

		servers.Reset();
		for (int32 index = 0; index < count; index++)
		{
			FServerListEntry entry;
			uint8 address[4] = {};
			uint8 up = 0;

			buffer.readUInt8(entry.serverId);
			for (int32 part = 0; part < 4; part++)
			{
				buffer.readUInt8(address[part]);
			}

			buffer.readInt32(entry.port);

			// Age limit, PvP.
			buffer.skip(2);

			buffer.readUInt16(entry.currentPlayers);
			buffer.readUInt16(entry.maxPlayers);
			buffer.readUInt8(up);

			// Server type bits, brackets.
			buffer.skip(5);

			if (buffer.hasFailed())
			{
				return false;
			}

			entry.name = FString::Printf(TEXT("Server %u"), entry.serverId);
			entry.host = FString::Printf(TEXT("%u.%u.%u.%u"), address[0], address[1], address[2], address[3]);
			entry.status = (up != 0) ? EServerStatus::Online : EServerStatus::Offline;
			servers.Add(entry);
		}

		return true;
	}
};

inline void writeAuthGameGuard(FWriteBuffer& buffer, int32 sessionId)
{
	buffer.writeUInt8(Opcode::authGameGuard);
	buffer.writeInt32(sessionId);

	// Where a real GameGuard answer would go. The server reads and discards four words.
	buffer.writeZeros(16);
}

/**
 * The credentials in one RSA block, at the offsets the server reads for the 128 byte form.
 * Account and password are cut to what the server reads, and the rest of the block is zero.
 */
inline bool writeRequestAuthLogin(FWriteBuffer& buffer, TArrayView<const uint8> modulus, const FString& account, const FString& password)
{
	static constexpr int32 accountOffset = 0x5E;
	static constexpr int32 accountBytes = 14;
	static constexpr int32 passwordOffset = 0x6C;
	static constexpr int32 passwordBytes = 16;

	TArray<uint8> block;
	block.SetNumZeroed(ServerCrypt::rsaBlockSize);

	const FTCHARToUTF8 accountUtf8(*account);
	const FTCHARToUTF8 passwordUtf8(*password);
	FMemory::Memcpy(block.GetData() + accountOffset, accountUtf8.Get(), FMath::Min(accountUtf8.Length(), accountBytes));
	FMemory::Memcpy(block.GetData() + passwordOffset, passwordUtf8.Get(), FMath::Min(passwordUtf8.Length(), passwordBytes));

	TArray<uint8> encrypted;
	const bool encryptedOk = ServerCrypt::rsaEncrypt(modulus, block, encrypted);

	// The password sat in this block in the clear.
	FMemory::Memzero(block.GetData(), block.Num());

	if (!encryptedOk)
	{
		return false;
	}

	buffer.writeUInt8(Opcode::requestAuthLogin);
	buffer.writeBytes(encrypted);
	return true;
}

inline void writeRequestServerList(FWriteBuffer& buffer, int32 loginOk1, int32 loginOk2)
{
	buffer.writeUInt8(Opcode::requestServerList);
	buffer.writeInt32(loginOk1);
	buffer.writeInt32(loginOk2);
}

inline void writeRequestServerLogin(FWriteBuffer& buffer, int32 loginOk1, int32 loginOk2, uint8 serverId)
{
	buffer.writeUInt8(Opcode::requestServerLogin);
	buffer.writeInt32(loginOk1);
	buffer.writeInt32(loginOk2);
	buffer.writeUInt8(serverId);
}
}
}
