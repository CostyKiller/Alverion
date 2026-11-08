#include "GameServer/GameServerPackets.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRequestHardWareInfoLayoutTest, "Alverion.Network.Packets.RequestHardWareInfo", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRequestHardWareInfoLayoutTest::RunTest(const FString& parameters)
{
	Network::FWriteBuffer buffer;
	Network::GameServer::writeRequestHardWareInfo(buffer);

	// Read it back field by field in the order the server reads it; any field out of place shifts the strings and fails the read.
	Network::FReadBuffer reader(buffer.bytes());
	uint8 opcode = 0;
	int16 extended = 0;
	FString machine;
	reader.readUInt8(opcode);
	reader.readInt16(extended);
	reader.readString(machine);

	int32 number = 0;
	for (int32 index = 0; index < 6; index++)
	{
		reader.readInt32(number);
	}
	reader.skip(16);

	FString processor;
	uint8 cores = 0;
	reader.readString(processor);
	reader.readInt32(number);
	reader.readUInt8(cores);
	reader.readInt32(number);

	for (int32 index = 0; index < 5; index++)
	{
		reader.readInt32(number);
	}

	uint8 unused = 0;
	int16 version = 0;
	FString videoCard;
	FString driver;
	reader.readUInt8(unused);
	reader.readInt32(number);
	reader.readInt32(number);
	reader.readInt16(version);
	reader.readString(videoCard);
	reader.readString(driver);

	TestEqual(TEXT("Extended opcode"), static_cast<int32>(opcode), 0xD0);
	TestEqual(TEXT("RequestHardWareInfo id"), static_cast<int32>(extended), 0x96);
	TestEqual(TEXT("The machine field is an MD5 in hex"), machine.Len(), 32);
	TestFalse(TEXT("Every field reads back"), reader.hasFailed());
	TestEqual(TEXT("Nothing left over"), reader.remaining(), 0);
	return true;
}

#endif
