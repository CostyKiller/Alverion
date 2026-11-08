#pragma once

#include "GameServer/GameServerProtocol.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "Misc/SecureHash.h"

namespace Network
{
namespace GameServer
{
/**
 * The machine description the stock client sends right after EnterWorld, in the order RequestHardWareInfo reads it.
 * A server with KickMissingHWID drops a player who has not sent it, about twenty seconds after entering.
 * The first field is where the stock client puts its MAC address; this sends a hash of the machine id instead, which is just as stable and gives nothing away.
 * Fields this client cannot know cheaply go as zero.
 */
inline void writeRequestHardWareInfo(FWriteBuffer& buffer)
{
	const int32 memoryMegabytes = static_cast<int32>(FPlatformMemory::GetConstants().TotalPhysical / (1024 * 1024));

	buffer.writeUInt8(Opcode::extended);
	buffer.writeInt16(Opcode::requestHardWareInfo);
	buffer.writeString(FMD5::HashAnsiString(*FPlatformMisc::GetLoginId()));

	// Windows platform id, major, minor and build, then DirectX version and revision.
	buffer.writeInt32(2);
	buffer.writeInt32(10);
	buffer.writeInt32(0);
	buffer.writeInt32(0);
	buffer.writeInt32(12);
	buffer.writeInt32(0);
	buffer.writeZeros(16);

	// Processor name, speed and cores, then an unused int.
	buffer.writeString(FPlatformMisc::GetCPUBrand().TrimStartAndEnd());
	buffer.writeInt32(0);
	buffer.writeUInt8(static_cast<uint8>(FMath::Clamp(FPlatformMisc::NumberOfCores(), 0, 255)));
	buffer.writeInt32(0);

	// Video card count and PCI speed, the three memory slots in megabytes, an unused byte, video memory, an unused int and the card version.
	buffer.writeInt32(1);
	buffer.writeInt32(0);
	buffer.writeInt32(memoryMegabytes);
	buffer.writeInt32(0);
	buffer.writeInt32(0);
	buffer.writeUInt8(0);
	buffer.writeInt32(0);
	buffer.writeInt32(0);
	buffer.writeInt16(0);

	// Video card name and driver version.
	buffer.writeString(FPlatformMisc::GetPrimaryGPUBrand().TrimStartAndEnd());
	buffer.writeString(FString());
}
}
}
