#include "ServerCrypt.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	TArray<uint8> sampleBytes(int32 count, int32 seed)
	{
		TArray<uint8> bytes;
		for (int32 index = 0; index < count; index++)
		{
			bytes.Add(static_cast<uint8>((index * 31) + seed));
		}

		return bytes;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameCipherRoundTripTest, "Alverion.Network.Crypt.GameCipher", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameCipherRoundTripTest::RunTest(const FString& parameters)
{
	const TArray<uint8> serverKey = sampleBytes(Network::FGameCipher::serverKeyLength, 7);
	Network::FGameCipher sender;
	Network::FGameCipher receiver;
	sender.setKey(serverKey);
	receiver.setKey(serverKey);

	// Several packets in a row, because the key rolls forward with every byte and a mismatch only shows on the second.
	for (int32 packet = 0; packet < 4; packet++)
	{
		const TArray<uint8> plain = sampleBytes(13 + (packet * 9), packet);
		TArray<uint8> wire = plain;
		sender.encrypt(wire);
		TestTrue(TEXT("The game cipher changes the bytes"), wire != plain);
		receiver.decrypt(wire);
		TestTrue(FString::Printf(TEXT("Packet %d survives the game cipher"), packet), wire == plain);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLoginCipherRoundTripTest, "Alverion.Network.Crypt.LoginCipher", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLoginCipherRoundTripTest::RunTest(const FString& parameters)
{
	const TArray<uint8> sessionKey = sampleBytes(16, 3);
	Network::FLoginCipher sender;
	Network::FLoginCipher receiver;
	sender.setSessionKey(sessionKey);
	receiver.setSessionKey(sessionKey);

	const TArray<uint8> plain = sampleBytes(41, 5);
	TArray<uint8> wire = plain;
	sender.encrypt(wire);

	TestEqual(TEXT("The login cipher pads to whole Blowfish blocks"), wire.Num() % 8, 0);
	TestTrue(TEXT("The checksum verifies after decryption"), receiver.decrypt(wire));
	TestTrue(TEXT("The body survives the login cipher"), TArray<uint8>(wire.GetData(), plain.Num()) == plain);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FXorPassRoundTripTest, "Alverion.Network.Crypt.XorPass", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FXorPassRoundTripTest::RunTest(const FString& parameters)
{
	const TArray<uint8> plain = sampleBytes(64, 11);
	TArray<uint8> raw = plain;
	Network::ServerCrypt::encXorPass(raw, 0x1234567);
	Network::ServerCrypt::decXorPass(raw);

	// The last eight bytes hold the pass's key afterwards, so only what comes before them is compared.
	TestTrue(TEXT("The XOR pass undoes itself"), TArray<uint8>(raw.GetData(), raw.Num() - 8) == TArray<uint8>(plain.GetData(), plain.Num() - 8));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModulusUnscrambleTest, "Alverion.Network.Crypt.Modulus", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FModulusUnscrambleTest::RunTest(const FString& parameters)
{
	const TArray<uint8> modulus = sampleBytes(Network::ServerCrypt::rsaBlockSize, 19);
	TArray<uint8> scrambled = modulus;

	// The login server's scramble, step for step, as ScrambledKeyPair writes it.
	for (int32 index = 0; index < 4; index++)
	{
		Swap(scrambled[index], scrambled[0x4D + index]);
	}

	for (int32 index = 0; index < 0x40; index++)
	{
		scrambled[index] ^= scrambled[0x40 + index];
	}

	for (int32 index = 0; index < 4; index++)
	{
		scrambled[0x0D + index] ^= scrambled[0x34 + index];
	}

	for (int32 index = 0; index < 0x40; index++)
	{
		scrambled[0x40 + index] ^= scrambled[index];
	}

	Network::ServerCrypt::unscrambleModulus(scrambled);
	TestTrue(TEXT("Unscrambling restores the modulus"), scrambled == modulus);
	return true;
}

#endif
