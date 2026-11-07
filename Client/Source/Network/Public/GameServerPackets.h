#pragma once

#include "Appearance.h"
#include "CoreMinimal.h"
#include "ReadBuffer.h"
#include "WriteBuffer.h"

/**
 * The game server's packets, as L2J Mobius writes and reads them, plus the appearance extension in ServerPatch.
 * Reads stop at the last field this client uses. Positions and headings become world centimetres and degrees here, so nothing past this header sees a server unit.
 */
namespace Network
{
/**
 * How long one server world unit is here.
 * A human's collision height is 23 units and it is a half height, so a 46 unit body against a 180cm MetaHuman puts a unit near 4cm.
 * Run speeds agree at that scale: 115 units a second is a 4.6m/s run.
 */
inline constexpr double worldUnitCm = 4.0;

inline double toCentimetres(int32 units)
{
	return static_cast<double>(units) * worldUnitCm;
}

inline int32 toUnits(double centimetres)
{
	return FMath::RoundToInt32(centimetres / worldUnitCm);
}

/** The sea surface around the starting island, rounded. The client puts it at Z zero, so heights are measured from the water. */
inline constexpr int32 seaLevelUnits = -3800;

inline FVector toLocation(int32 x, int32 y, int32 z)
{
	return FVector(toCentimetres(x), toCentimetres(y), toCentimetres(z - seaLevelUnits));
}

inline int32 toHeightUnits(double centimetres)
{
	return toUnits(centimetres) + seaLevelUnits;
}

/** Server headings run 0 to 65535 for a full turn, in the same direction as yaw. */
inline float headingToYaw(int32 heading)
{
	return static_cast<float>(heading & 0xFFFF) * (360.0f / 65536.0f);
}

inline int32 yawToHeading(float yaw)
{
	const float wrapped = FMath::Fmod(FMath::Fmod(yaw, 360.0f) + 360.0f, 360.0f);
	return FMath::RoundToInt32(wrapped * (65536.0f / 360.0f)) & 0xFFFF;
}

enum class ERace : int32
{
	Human = 0,
	Elf = 1,
	DarkElf = 2,
	Orc = 3,
	Dwarf = 4,
	Kamael = 5
};

/** CharCreateFail's codes. */
enum class ECreateRejectReason : int32
{
	CreationFailed = 0,
	TooManyCharacters = 1,
	NameTaken = 2,
	NameTooLong = 3,
	NameInvalid = 4,
	CreationNotAllowed = 5,
	ChooseAnotherServer = 6
};

inline const TCHAR* createRejectReasonName(ECreateRejectReason reason)
{
	switch (reason)
	{
		case ECreateRejectReason::TooManyCharacters:
		{
			return TEXT("This account cannot hold another character.");
		}
		case ECreateRejectReason::NameTaken:
		{
			return TEXT("That name is taken.");
		}
		case ECreateRejectReason::NameTooLong:
		{
			return TEXT("That name is too long.");
		}
		case ECreateRejectReason::NameInvalid:
		{
			return TEXT("Names are 2 to 16 letters and digits.");
		}
		default:
		{
			return TEXT("The character could not be created.");
		}
	}
}

struct NETWORK_API FCharacterListEntry
{
	uint32 objectId = 0;

	/** The server picks characters by their place in the list, not by id. */
	int32 slot = 0;

	FString name;
	int32 level = 0;
	bool female = false;
	int32 race = 0;
	int32 classId = 0;
	int32 hairStyle = 0;
	int32 hairColor = 0;
	int32 face = 0;

	/** Seconds until a character marked for deletion goes, zero for one that is not. */
	int32 deleteSeconds = 0;

	FVector location = FVector::ZeroVector;

	/** The extension's bytes, empty for a character made without them. */
	TArray<uint8> appearance;
};

struct NETWORK_API FCharacterSelected
{
	uint32 objectId = 0;
	FString name;
	FString title;
	int32 level = 0;
	bool female = false;
	int32 race = 0;
	int32 classId = 0;
	FVector location = FVector::ZeroVector;
	int32 currentHp = 0;
};

/** Somebody or something coming into view: a player from CharInfo, or a monster or townsperson from NpcInfo. */
struct NETWORK_API FCharacterSpawn
{
	uint32 objectId = 0;
	FString name;
	FString title;
	FVector location = FVector::ZeroVector;
	float yaw = 0.0f;

	/** Centimetres a second, already multiplied by the server's move multiplier. */
	float runSpeed = 0.0f;

	bool isNpc = false;
	int32 npcId = 0;
	bool attackable = false;

	bool female = false;
	int32 race = 0;
	int32 hairStyle = 0;
	int32 hairColor = 0;
	int32 face = 0;
	TArray<uint8> appearance;
};

/** An object id and a place, which is all MoveToLocation, StopMove, ValidateLocation and TeleportToLocation carry. */
struct NETWORK_API FObjectPlace
{
	uint32 objectId = 0;
	FVector location = FVector::ZeroVector;
	float yaw = 0.0f;
};

namespace GameServer
{
/** The newest protocol revision the server accepts. */
inline constexpr int32 protocolRevision = 273;

/** The paperdoll slots CharSelectionInfo and CharInfo list, one int each. */
inline constexpr int32 paperdollSlots = 26;

/** Marks the appearance extension, as the bytes 'A' 'L' 'V' '1'. A server without the patch never sends it and ignores it when sent. */
inline constexpr int32 appearanceMagic = 0x31564C41;

namespace Opcode
{
	// Server to client.
	inline constexpr uint32 deleteObject = 0x08;
	inline constexpr uint32 characterSelectionInfo = 0x09;
	inline constexpr uint32 loginFail = 0x0A;
	inline constexpr uint32 characterSelected = 0x0B;
	inline constexpr uint32 npcInfo = 0x0C;
	inline constexpr uint32 characterCreateSuccess = 0x0F;
	inline constexpr uint32 characterCreateFail = 0x10;
	inline constexpr uint32 characterDeleteSuccess = 0x1D;
	inline constexpr uint32 characterDeleteFail = 0x1E;
	inline constexpr uint32 teleportToLocation = 0x22;
	inline constexpr uint32 versionCheck = 0x2E;
	inline constexpr uint32 moveToLocation = 0x2F;
	inline constexpr uint32 charInfo = 0x31;
	inline constexpr uint32 userInfo = 0x32;
	inline constexpr uint32 stopMove = 0x47;
	inline constexpr uint32 validateLocation = 0x79;

	// Client to server.
	inline constexpr uint8 logout = 0x00;
	inline constexpr uint8 characterCreate = 0x0C;
	inline constexpr uint8 characterDelete = 0x0D;
	inline constexpr uint8 protocolVersion = 0x0E;
	inline constexpr uint8 moveBackwardToLocation = 0x0F;
	inline constexpr uint8 enterWorld = 0x11;
	inline constexpr uint8 characterSelect = 0x12;
	inline constexpr uint8 authLogin = 0x2B;
	inline constexpr uint8 appearing = 0x3A;
	inline constexpr uint8 validatePosition = 0x59;

	// Extended client packets: this byte, then a uint16 naming the packet.
	inline constexpr uint8 extended = 0xD0;
	inline constexpr int16 requestGotoLobby = 0x36;
}

/**
 * The extension's trailer, read from the end of a packet so nothing before it has to be understood:
 * count blobs of size bytes each, then size and count as int16, then the magic.
 */
inline bool readAppearanceTrailer(TArrayView<const uint8> body, TArray<TArray<uint8>>& blobs)
{
	static constexpr int32 footerBytes = 8;

	blobs.Reset();
	if (body.Num() < footerBytes)
	{
		return false;
	}

	FReadBuffer footer(TArrayView<const uint8>(body.GetData() + body.Num() - footerBytes, footerBytes));
	int16 size = 0;
	int16 count = 0;
	int32 magic = 0;
	footer.readInt16(size);
	footer.readInt16(count);
	footer.readInt32(magic);

	const int32 blobBytes = static_cast<int32>(size) * static_cast<int32>(count);
	if ((magic != appearanceMagic) || (size <= 0) || (count <= 0) || (blobBytes > (body.Num() - footerBytes)))
	{
		return false;
	}

	const uint8* start = body.GetData() + body.Num() - footerBytes - blobBytes;
	for (int32 index = 0; index < count; index++)
	{
		blobs.Emplace(start + (index * size), size);
	}

	return true;
}

/** The key, and whether the server encrypts at all, which L2J leaves off by default and announces in the int after the key. */
inline bool readKeyPacket(FReadBuffer& buffer, TArray<uint8>& key, bool& encrypted)
{
	uint8 result = 0;
	int32 encryption = 0;
	buffer.readUInt8(result);
	buffer.readBytes(key, 8);
	buffer.readInt32(encryption);

	encrypted = (encryption != 0);
	return !buffer.hasFailed() && (result == 1);
}

inline bool readCharacterSelectionInfo(TArrayView<const uint8> body, TArray<FCharacterListEntry>& characters)
{
	FReadBuffer buffer(body);
	int32 count = 0;
	buffer.readInt32(count);

	// The account's character limit, then a byte.
	buffer.skip(5);

	characters.Reset();
	for (int32 slot = 0; (slot < count) && !buffer.hasFailed(); slot++)
	{
		FCharacterListEntry entry;
		FString accountName;
		int32 objectId = 0;
		int32 female = 0;
		int32 x = 0;
		int32 y = 0;
		int32 z = 0;

		entry.slot = slot;
		buffer.readString(entry.name);
		buffer.readInt32(objectId);
		buffer.readString(accountName);

		// Session id, clan, builder level.
		buffer.skip(12);

		buffer.readInt32(female);
		buffer.readInt32(entry.race);

		// Base class, server.
		buffer.skip(8);

		buffer.readInt32(x);
		buffer.readInt32(y);
		buffer.readInt32(z);

		// HP and MP as doubles, SP, experience, experience percent.
		buffer.skip(8 + 8 + 4 + 8 + 8);

		buffer.readInt32(entry.level);

		// Karma, PK and PvP kills, seven zeros, then the paperdoll.
		buffer.skip((10 + paperdollSlots) * 4);

		buffer.readInt32(entry.hairStyle);
		buffer.readInt32(entry.hairColor);
		buffer.readInt32(entry.face);

		// Max HP and MP.
		buffer.skip(16);

		buffer.readInt32(entry.deleteSeconds);
		buffer.readInt32(entry.classId);

		// Active, enchant byte, augmentation, transformation, four pet ints, two pet doubles, vitality.
		buffer.skip(4 + 1 + (6 * 4) + 16 + 4);

		entry.objectId = static_cast<uint32>(objectId);
		entry.female = (female != 0);
		entry.location = toLocation(x, y, z);
		characters.Add(entry);
	}

	if (buffer.hasFailed())
	{
		return false;
	}

	TArray<TArray<uint8>> blobs;
	if (readAppearanceTrailer(body, blobs) && (blobs.Num() == characters.Num()))
	{
		for (int32 index = 0; index < blobs.Num(); index++)
		{
			FAppearance check;
			if (FAppearance::unpack(blobs[index], check))
			{
				characters[index].appearance = blobs[index];
			}
		}
	}

	return true;
}

inline bool readCharacterSelected(FReadBuffer& buffer, FCharacterSelected& selected)
{
	int32 objectId = 0;
	int32 female = 0;
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;
	double hp = 0.0;

	buffer.readString(selected.name);
	buffer.readInt32(objectId);
	buffer.readString(selected.title);

	// Session id, clan, an unknown.
	buffer.skip(12);

	buffer.readInt32(female);
	buffer.readInt32(selected.race);
	buffer.readInt32(selected.classId);

	// Active.
	buffer.skip(4);

	buffer.readInt32(x);
	buffer.readInt32(y);
	buffer.readInt32(z);
	buffer.readDouble(hp);

	// MP, SP, experience.
	buffer.skip(8 + 4 + 8);

	buffer.readInt32(selected.level);

	if (buffer.hasFailed())
	{
		return false;
	}

	selected.objectId = static_cast<uint32>(objectId);
	selected.female = (female != 0);
	selected.location = toLocation(x, y, z);
	selected.currentHp = FMath::RoundToInt32(hp);
	return true;
}

/** UserInfo's head, which is enough to know this client's own object id. */
inline bool readUserInfoObjectId(FReadBuffer& buffer, uint32& objectId)
{
	int32 id = 0;

	// Position and vehicle.
	buffer.skip(16);
	buffer.readInt32(id);

	objectId = static_cast<uint32>(id);
	return !buffer.hasFailed();
}

/** CharInfo: another player, read as far as its heading, then the extension's trailer from the end. */
inline bool readCharInfo(TArrayView<const uint8> body, FCharacterSpawn& spawn)
{
	FReadBuffer buffer(body);
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;
	int32 objectId = 0;
	int32 female = 0;
	int32 runSpeed = 0;
	double moveMultiplier = 1.0;
	uint16 cubicCount = 0;
	int32 heading = 0;

	buffer.readInt32(x);
	buffer.readInt32(y);
	buffer.readInt32(z);

	// Vehicle.
	buffer.skip(4);

	buffer.readInt32(objectId);
	buffer.readString(spawn.name);
	buffer.readInt32(spawn.race);
	buffer.readInt32(female);

	// Base class, paperdoll items and their augmentations, then talismans, cloak, PvP flag, karma, both attack speeds and an unknown.
	buffer.skip((1 + (paperdollSlots * 2) + 7) * 4);

	buffer.readInt32(runSpeed);

	// Walk, swim run, swim walk, flying run and walk twice over.
	buffer.skip(7 * 4);

	buffer.readDouble(moveMultiplier);

	// Attack speed multiplier, collision radius and height.
	buffer.skip(24);

	buffer.readInt32(spawn.hairStyle);
	buffer.readInt32(spawn.hairColor);
	buffer.readInt32(spawn.face);
	buffer.readString(spawn.title);

	// Clan, crest, alliance, alliance crest, then standing, running, combat, dead, invisible, mount and store as bytes.
	buffer.skip((4 * 4) + 7);

	buffer.readUInt16(cubicCount);
	buffer.skip(cubicCount * 2);

	// Matching room, abnormal effects, water or flying, recommendations, mount npc, class, an unknown, enchant glow, team, large crest, noble, hero, fishing, fishing spot, name colour.
	buffer.skip(1 + 4 + 1 + 2 + (3 * 4) + 1 + 1 + 4 + 1 + 1 + 1 + (4 * 4));

	buffer.readInt32(heading);

	if (buffer.hasFailed())
	{
		return false;
	}

	spawn.objectId = static_cast<uint32>(objectId);
	spawn.female = (female != 0);
	spawn.location = toLocation(x, y, z);
	spawn.yaw = headingToYaw(heading);
	spawn.runSpeed = static_cast<float>(toCentimetres(runSpeed) * moveMultiplier);

	TArray<TArray<uint8>> blobs;
	FAppearance check;
	if (readAppearanceTrailer(body, blobs) && (blobs.Num() == 1) && FAppearance::unpack(blobs[0], check))
	{
		spawn.appearance = blobs[0];
	}

	return true;
}

/** NpcInfo: a monster, a townsperson, a summon or a trap, read as far as its name. */
inline bool readNpcInfo(FReadBuffer& buffer, FCharacterSpawn& spawn)
{
	static constexpr int32 npcIdOffset = 1000000;

	int32 objectId = 0;
	int32 displayId = 0;
	int32 attackable = 0;
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;
	int32 heading = 0;
	int32 runSpeed = 0;
	double moveMultiplier = 1.0;

	buffer.readInt32(objectId);
	buffer.readInt32(displayId);
	buffer.readInt32(attackable);
	buffer.readInt32(x);
	buffer.readInt32(y);
	buffer.readInt32(z);
	buffer.readInt32(heading);

	// An unknown, both attack speeds.
	buffer.skip(12);

	buffer.readInt32(runSpeed);

	// Walk, swim run, swim walk, flying run and walk twice over.
	buffer.skip(7 * 4);

	buffer.readDouble(moveMultiplier);

	// Attack speed multiplier, collision radius and height, three equipment ids, five flags, the client name string id.
	buffer.skip(24 + 12 + 5 + 4);

	buffer.readString(spawn.name);

	if (buffer.hasFailed())
	{
		return false;
	}

	spawn.objectId = static_cast<uint32>(objectId);
	spawn.isNpc = true;
	spawn.npcId = displayId - npcIdOffset;
	spawn.attackable = (attackable != 0);
	spawn.location = toLocation(x, y, z);
	spawn.yaw = headingToYaw(heading);
	spawn.runSpeed = static_cast<float>(toCentimetres(runSpeed) * moveMultiplier);

	if (spawn.name.IsEmpty())
	{
		spawn.name = FString::Printf(TEXT("Npc %d"), spawn.npcId);
	}

	return true;
}

/** The object id and position every place packet starts with. MoveToLocation's is the destination. */
inline bool readObjectPlace(FReadBuffer& buffer, FObjectPlace& place)
{
	int32 objectId = 0;
	int32 x = 0;
	int32 y = 0;
	int32 z = 0;

	buffer.readInt32(objectId);
	buffer.readInt32(x);
	buffer.readInt32(y);
	buffer.readInt32(z);

	place.objectId = static_cast<uint32>(objectId);
	place.location = toLocation(x, y, z);
	return !buffer.hasFailed();
}

/** StopMove and ValidateLocation end in a heading. TeleportToLocation has a fade flag before it. */
inline bool readObjectPlaceWithHeading(FReadBuffer& buffer, FObjectPlace& place, bool teleport)
{
	int32 heading = 0;

	readObjectPlace(buffer, place);
	if (teleport)
	{
		buffer.skip(4);
	}

	buffer.readInt32(heading);
	place.yaw = headingToYaw(heading);
	return !buffer.hasFailed();
}

inline void writeProtocolVersion(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::protocolVersion);
	buffer.writeInt32(protocolRevision);
}

/** The order is the server's: play key two before play key one, then the login pair in order. */
inline void writeAuthLogin(FWriteBuffer& buffer, const FString& account, int32 playOk1, int32 playOk2, int32 loginOk1, int32 loginOk2)
{
	buffer.writeUInt8(Opcode::authLogin);
	buffer.writeString(account.ToLower());
	buffer.writeInt32(playOk2);
	buffer.writeInt32(playOk1);
	buffer.writeInt32(loginOk1);
	buffer.writeInt32(loginOk2);
}

/**
 * The stock fields, then the extension's appearance after the magic, which an unpatched server never reads.
 * The six base stats the server ignores are zero, and the race it keeps comes from the class template.
 */
inline void writeCharacterCreate(FWriteBuffer& buffer, const FString& name, bool female, int32 classId, int32 hairStyle, int32 hairColor, int32 face, TArrayView<const uint8> appearance)
{
	buffer.writeUInt8(Opcode::characterCreate);
	buffer.writeString(name);
	buffer.writeInt32(static_cast<int32>(ERace::Human));
	buffer.writeInt32(female ? 1 : 0);
	buffer.writeInt32(classId);
	buffer.writeZeros(6 * 4);
	buffer.writeInt32(hairStyle);
	buffer.writeInt32(hairColor);
	buffer.writeInt32(face);

	buffer.writeInt32(appearanceMagic);
	buffer.writeUInt8(static_cast<uint8>(appearance.Num()));
	buffer.writeBytes(appearance);
}

inline void writeCharacterDelete(FWriteBuffer& buffer, int32 slot)
{
	buffer.writeUInt8(Opcode::characterDelete);
	buffer.writeInt32(slot);
}

inline void writeCharacterSelect(FWriteBuffer& buffer, int32 slot)
{
	buffer.writeUInt8(Opcode::characterSelect);
	buffer.writeInt32(slot);
	buffer.writeZeros(2 + 12);
}

/** Eighty four bytes the server throws away, then twenty it reads as a traceroute. A shorter packet makes it read past the end and drop the entry. */
inline void writeEnterWorld(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::enterWorld);
	buffer.writeZeros(84 + 20);
}

/** Keyboard movement is mode zero, which is what makes the server take ValidatePosition as where the character is. */
inline void writeMoveToLocation(FWriteBuffer& buffer, const FVector& target, const FVector& origin)
{
	buffer.writeUInt8(Opcode::moveBackwardToLocation);
	buffer.writeInt32(toUnits(target.X));
	buffer.writeInt32(toUnits(target.Y));
	buffer.writeInt32(toHeightUnits(target.Z));
	buffer.writeInt32(toUnits(origin.X));
	buffer.writeInt32(toUnits(origin.Y));
	buffer.writeInt32(toHeightUnits(origin.Z));
	buffer.writeInt32(0);
}

inline void writeValidatePosition(FWriteBuffer& buffer, const FVector& location, float yaw)
{
	buffer.writeUInt8(Opcode::validatePosition);
	buffer.writeInt32(toUnits(location.X));
	buffer.writeInt32(toUnits(location.Y));
	buffer.writeInt32(toHeightUnits(location.Z));
	buffer.writeInt32(yawToHeading(yaw));
	buffer.writeInt32(0);
}

/** Sent after every TeleportToLocation aimed at this client. Until it arrives the server holds the character as teleporting and ignores its positions. */
inline void writeAppearing(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::appearing);
}

/** Asks for the character list again. The server answers creation with CharCreateOk alone and waits for this. */
inline void writeRequestGotoLobby(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::extended);
	buffer.writeInt16(Opcode::requestGotoLobby);
}

inline void writeLogout(FWriteBuffer& buffer)
{
	buffer.writeUInt8(Opcode::logout);
}
}
}
