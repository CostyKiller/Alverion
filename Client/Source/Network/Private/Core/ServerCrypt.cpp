#include "Core/ServerCrypt.h"

#include "Core/OpenSslIncludes.h"

namespace Network
{

namespace
{
	// Shared by every login server and client of this protocol. It only ever protects Init, which carries the real key.
	const uint8 staticLoginKey[16] = { 0x6B, 0x60, 0xCB, 0x5B, 0x82, 0xCE, 0x90, 0xB1, 0xCC, 0x2B, 0x6C, 0x55, 0x6C, 0x6C, 0x6C, 0x6C };

	// The half of the game key the server never sends.
	const uint8 gameKeyTail[8] = { 0xC8, 0x27, 0x93, 0x01, 0xA1, 0x6C, 0x31, 0x97 };

	// Where the game cipher keeps its rolling count of bytes, as a little-endian int inside the key itself.
	constexpr int32 gameKeyCounterOffset = 8;

	constexpr int32 blowfishBlockSize = 8;

	// The login server's RSA key is generated with the usual public exponent, and only the modulus is sent.
	constexpr BN_ULONG rsaExponent = 65537;
	constexpr int32 checksumSize = 4;

	uint32 readWord(const uint8* bytes)
	{
		return static_cast<uint32>(bytes[0]) | (static_cast<uint32>(bytes[1]) << 8) | (static_cast<uint32>(bytes[2]) << 16) | (static_cast<uint32>(bytes[3]) << 24);
	}

	void writeWord(uint8* bytes, uint32 value)
	{
		bytes[0] = static_cast<uint8>(value);
		bytes[1] = static_cast<uint8>(value >> 8);
		bytes[2] = static_cast<uint8>(value >> 16);
		bytes[3] = static_cast<uint8>(value >> 24);
	}

	// OpenSSL takes each half block as a native word, so the little-endian load and store are what make it this protocol's Blowfish.
	void blowfishBlocks(TArray<uint8>& bytes, const BF_KEY& key, bool encrypting)
	{
		for (int32 offset = 0; (offset + blowfishBlockSize) <= bytes.Num(); offset += blowfishBlockSize)
		{
			BF_LONG block[2] = { readWord(bytes.GetData() + offset), readWord(bytes.GetData() + offset + 4) };

			if (encrypting)
			{
				BF_encrypt(block, &key);
			}
			else
			{
				BF_decrypt(block, &key);
			}

			writeWord(bytes.GetData() + offset, static_cast<uint32>(block[0]));
			writeWord(bytes.GetData() + offset + 4, static_cast<uint32>(block[1]));
		}
	}

	void advanceGameKey(uint8* key, int32 size)
	{
		const uint32 counter = readWord(key + gameKeyCounterOffset) + static_cast<uint32>(size);
		writeWord(key + gameKeyCounterOffset, counter);
	}
}

struct FLoginCipher::FKeys
{
	BF_KEY staticKey;
	BF_KEY sessionKey;
};

FLoginCipher::FLoginCipher()
	: _keys(MakeUnique<FKeys>())
{
	BF_set_key(&_keys->staticKey, UE_ARRAY_COUNT(staticLoginKey), staticLoginKey);
}

FLoginCipher::~FLoginCipher() = default;

void FLoginCipher::setSessionKey(TArrayView<const uint8> key)
{
	BF_set_key(&_keys->sessionKey, key.Num(), key.GetData());
	_hasSessionKey = true;
}

void FLoginCipher::encrypt(TArray<uint8>& body)
{
	// Room for the checksum, then up to a whole Blowfish block, zero filled. The server reads the padding as part of the packet and ignores it.
	const int32 size = Align(body.Num() + checksumSize, blowfishBlockSize);
	body.SetNumZeroed(size);

	ServerCrypt::appendChecksum(body);
	blowfishBlocks(body, _hasSessionKey ? _keys->sessionKey : _keys->staticKey, true);
}

bool FLoginCipher::decrypt(TArray<uint8>& body)
{
	if ((body.Num() <= checksumSize) || ((body.Num() % blowfishBlockSize) != 0))
	{
		return false;
	}

	if (!_hasSessionKey)
	{
		// Init. No checksum, and the XOR pass to undo after the cipher.
		blowfishBlocks(body, _keys->staticKey, false);
		ServerCrypt::decXorPass(body);
		return true;
	}

	blowfishBlocks(body, _keys->sessionKey, false);
	return ServerCrypt::verifyChecksum(body);
}

void FGameCipher::setKey(TArrayView<const uint8> serverKey)
{
	check(serverKey.Num() >= serverKeyLength);

	FMemory::Memcpy(_inKey, serverKey.GetData(), serverKeyLength);
	FMemory::Memcpy(_inKey + serverKeyLength, gameKeyTail, UE_ARRAY_COUNT(gameKeyTail));
	FMemory::Memcpy(_outKey, _inKey, keyLength);

	_enabled = true;
}

bool FGameCipher::isEnabled() const
{
	return _enabled;
}

void FGameCipher::encrypt(TArray<uint8>& body)
{
	if (!_enabled || (body.Num() <= 0))
	{
		return;
	}

	uint8 previous = 0;
	for (int32 index = 0; index < body.Num(); index++)
	{
		previous = body[index] ^ _outKey[index & (keyLength - 1)] ^ previous;
		body[index] = previous;
	}

	advanceGameKey(_outKey, body.Num());
}

bool FGameCipher::decrypt(TArray<uint8>& body)
{
	if (!_enabled || (body.Num() <= 0))
	{
		return true;
	}

	uint8 previous = 0;
	for (int32 index = 0; index < body.Num(); index++)
	{
		const uint8 encrypted = body[index];
		body[index] = encrypted ^ _inKey[index & (keyLength - 1)] ^ previous;
		previous = encrypted;
	}

	advanceGameKey(_inKey, body.Num());
	return true;
}

namespace ServerCrypt
{

void unscrambleModulus(TArray<uint8>& modulus)
{
	check(modulus.Num() == rsaBlockSize);

	// The server's four steps, undone in reverse order.
	for (int32 index = 0; index < 0x40; index++)
	{
		modulus[0x40 + index] ^= modulus[index];
	}

	for (int32 index = 0; index < 4; index++)
	{
		modulus[0x0D + index] ^= modulus[0x34 + index];
	}

	for (int32 index = 0; index < 0x40; index++)
	{
		modulus[index] ^= modulus[0x40 + index];
	}

	for (int32 index = 0; index < 4; index++)
	{
		Swap(modulus[index], modulus[0x4D + index]);
	}
}

bool rsaEncrypt(TArrayView<const uint8> modulus, TArrayView<const uint8> block, TArray<uint8>& out)
{
	if ((modulus.Num() != rsaBlockSize) || (block.Num() != rsaBlockSize))
	{
		return false;
	}

	BN_CTX* context = BN_CTX_new();
	BIGNUM* n = BN_bin2bn(modulus.GetData(), modulus.Num(), nullptr);
	BIGNUM* m = BN_bin2bn(block.GetData(), block.Num(), nullptr);
	BIGNUM* e = BN_new();
	BIGNUM* c = BN_new();

	bool encrypted = (context != nullptr) && (n != nullptr) && (m != nullptr) && (e != nullptr) && (c != nullptr);
	encrypted = encrypted && (BN_set_word(e, rsaExponent) == 1);
	encrypted = encrypted && (BN_mod_exp(c, m, e, n, context) == 1);

	if (encrypted)
	{
		out.SetNumZeroed(rsaBlockSize);
		encrypted = (BN_bn2binpad(c, out.GetData(), rsaBlockSize) == rsaBlockSize);
	}

	BN_free(c);
	BN_free(e);
	BN_free(m);
	BN_free(n);
	BN_CTX_free(context);

	return encrypted;
}

void appendChecksum(TArray<uint8>& raw)
{
	check(((raw.Num() % 4) == 0) && (raw.Num() > checksumSize));

	uint32 checksum = 0;
	const int32 last = raw.Num() - checksumSize;
	for (int32 offset = 0; offset < last; offset += 4)
	{
		checksum ^= readWord(raw.GetData() + offset);
	}

	writeWord(raw.GetData() + last, checksum);
}

bool verifyChecksum(TArrayView<const uint8> raw)
{
	if (((raw.Num() % 4) != 0) || (raw.Num() <= checksumSize))
	{
		return false;
	}

	uint32 checksum = 0;
	const int32 last = raw.Num() - checksumSize;
	for (int32 offset = 0; offset < last; offset += 4)
	{
		checksum ^= readWord(raw.GetData() + offset);
	}

	return checksum == readWord(raw.GetData() + last);
}

void encXorPass(TArray<uint8>& raw, int32 key)
{
	const int32 end = raw.Num() - 8;
	int32 offset = 4;
	uint32 rolling = static_cast<uint32>(key);

	while (offset < end)
	{
		uint32 word = readWord(raw.GetData() + offset);
		rolling += word;
		word ^= rolling;
		writeWord(raw.GetData() + offset, word);
		offset += 4;
	}

	writeWord(raw.GetData() + offset, rolling);
}

void decXorPass(TArray<uint8>& raw)
{
	if (raw.Num() < 16)
	{
		return;
	}

	uint32 rolling = readWord(raw.GetData() + raw.Num() - 8);
	for (int32 offset = raw.Num() - 12; offset >= 4; offset -= 4)
	{
		uint32 word = readWord(raw.GetData() + offset);
		word ^= rolling;
		rolling -= word;
		writeWord(raw.GetData() + offset, word);
	}
}

}

}
