#include "Core/WriteBuffer.h"
#include "GameServer/GameServerPackets.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStatusUpdateTest, "Alverion.Network.Packets.StatusUpdate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStatusUpdateTest::RunTest(const FString& parameters)
{
	// A body as StatusUpdate.java writes it: object id, count, then id and value pairs, with one attribute this client does not follow.
	Network::FWriteBuffer body;
	body.writeInt32(268480000);
	body.writeInt32(3);
	body.writeInt32(Network::GameServer::StatusAttribute::currentHp);
	body.writeInt32(57);
	body.writeInt32(0x1B);
	body.writeInt32(999);
	body.writeInt32(Network::GameServer::StatusAttribute::maxMp);
	body.writeInt32(80);

	Network::FReadBuffer reader(body.bytes());
	Network::FStatusUpdate update;
	TestTrue(TEXT("The update reads"), Network::GameServer::readStatusUpdate(reader, update));
	TestEqual(TEXT("Nothing left over"), reader.remaining(), 0);
	TestEqual(TEXT("Object id"), update.objectId, static_cast<uint32>(268480000));
	TestEqual(TEXT("Every pair read"), update.attributes.Num(), 3);

	Network::FUserInfo info;
	info.currentHp = 100;
	info.maxHp = 150;
	info.currentMp = 40;
	Network::GameServer::applyStatusUpdate(update, info);
	TestEqual(TEXT("Current HP follows the update"), info.currentHp, 57);
	TestEqual(TEXT("Max HP stays as it was"), info.maxHp, 150);
	TestEqual(TEXT("Current MP stays as it was"), info.currentMp, 40);
	TestEqual(TEXT("Max MP follows the update"), info.maxMp, 80);

	// A count longer than the body fails the read rather than inventing values.
	Network::FWriteBuffer shortBody;
	shortBody.writeInt32(1);
	shortBody.writeInt32(3);
	shortBody.writeInt32(Network::GameServer::StatusAttribute::currentHp);
	shortBody.writeInt32(1);
	Network::FReadBuffer shortReader(shortBody.bytes());
	Network::FStatusUpdate broken;
	TestFalse(TEXT("A short body fails"), Network::GameServer::readStatusUpdate(shortReader, broken));
	return true;
}

#endif
