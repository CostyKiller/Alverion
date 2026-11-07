#pragma once

#include "LoginServer/LoginServerProtocol.h"

namespace Network
{
namespace LoginServer
{
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
}
}
