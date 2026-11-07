#pragma once

#include "CoreMinimal.h"

namespace Network
{

// What a session runs every body through, once per frame and in arrival order.
// Both ciphers carry state from one frame to the next, so a frame skipped or reordered breaks every frame after it.
class NETWORK_API ICipher
{
public:
	virtual ~ICipher() = default;

	// An outgoing body, opcode first, made into what follows the length on the wire. It may grow.
	virtual void encrypt(TArray<uint8>& body) = 0;

	// A received body made plain again, opcode first. False means it cannot be trusted and the link should die.
	virtual bool decrypt(TArray<uint8>& body) = 0;
};

// The login link.
// Blowfish over little-endian words, which is the one place the protocol departs from the cipher as published.
// The first frame the server sends is Init, under a static key and an extra XOR pass, and it carries the key every later frame uses.
class NETWORK_API FLoginCipher final : public ICipher
{
public:
	FLoginCipher();
	virtual ~FLoginCipher() override;

	virtual void encrypt(TArray<uint8>& body) override;
	virtual bool decrypt(TArray<uint8>& body) override;

	// The sixteen bytes Init carried. Every frame after Init, both ways, uses them.
	void setSessionKey(TArrayView<const uint8> key);

private:
	struct FKeys;
	TUniquePtr<FKeys> _keys;
	bool _hasSessionKey = false;
};

// The game link.
// An XOR stream over a sixteen byte key: eight from the server's KeyPacket, eight that every client and server share.
// Off until the key arrives, because ProtocolVersion goes out and KeyPacket comes back in the clear.
class NETWORK_API FGameCipher final : public ICipher
{
public:
	static constexpr int32 keyLength = 16;
	static constexpr int32 serverKeyLength = 8;

	virtual void encrypt(TArray<uint8>& body) override;
	virtual bool decrypt(TArray<uint8>& body) override;

	void setKey(TArrayView<const uint8> serverKey);
	bool isEnabled() const;

private:
	uint8 _inKey[keyLength] = {};
	uint8 _outKey[keyLength] = {};
	bool _enabled = false;
};

// The pieces of the login exchange that are plain functions, public so the tests can reach them.
namespace ServerCrypt
{
	inline constexpr int32 rsaBlockSize = 128;

	// Undoes the shuffle the login server applies to its RSA modulus before sending it in Init.
	NETWORK_API void unscrambleModulus(TArray<uint8>& modulus);

	// RSA with no padding, the way the login server decrypts: a 128 byte block raised to 65537 modulo the modulus, both big endian.
	NETWORK_API bool rsaEncrypt(TArrayView<const uint8> modulus, TArrayView<const uint8> block, TArray<uint8>& out);

	// The XOR of every little-endian word but the last, written into the last. The size must be a multiple of four.
	NETWORK_API void appendChecksum(TArray<uint8>& raw);
	NETWORK_API bool verifyChecksum(TArrayView<const uint8> raw);

	// The pass the server runs over Init before the static Blowfish. Its key travels in the frame, eight bytes from the end.
	NETWORK_API void encXorPass(TArray<uint8>& raw, int32 key);
	NETWORK_API void decXorPass(TArray<uint8>& raw);
}

}
