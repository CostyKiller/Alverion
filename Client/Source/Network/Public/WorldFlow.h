#pragma once

#include "CoreMinimal.h"
#include "GameServerFlow.h"
#include "GameServerPackets.h"

namespace Network
{
/** Where somebody else is headed or has stopped. Destinations are walked to at the mover's speed; stops are where it now stands. */
enum class EMoveKind : uint8
{
	Destination,
	Stopped
};

DECLARE_DELEGATE_OneParam(FCharacterSpawned, const FCharacterSpawn&);
DECLARE_DELEGATE_OneParam(FCharacterDespawned, uint32);
DECLARE_DELEGATE_TwoParams(FCharacterMoved, const FObjectPlace&, EMoveKind);
DECLARE_DELEGATE_TwoParams(FLocalCorrected, const FObjectPlace&, bool);

/**
 * The world, from EnterWorld on.
 * The local character moves itself and reports in keyboard mode, which makes the server take its positions as true.
 * Everybody else arrives as spawns, destinations, stops and corrections.
 */
class NETWORK_API FWorldFlow
{
public:
	~FWorldFlow();

	/** Sends EnterWorld. Called once the world exists, because everything the server says after it needs somewhere to land. */
	bool begin(FGameServerFlow& gameServer);
	void end();

	void setLocalState(const FVector& location, float yaw, bool wantsToMove);
	void tick(float deltaSeconds);

	bool isActive() const;

	FCharacterSpawned onSpawned;
	FCharacterDespawned onDespawned;
	FCharacterMoved onMoved;

	/** The server moving this client's own character. True for a teleport, which snaps rather than blends. */
	FLocalCorrected onCorrected;

private:
	void handlePacket(uint32 opcode, TArrayView<const uint8> body);
	void sendMove(const FVector& target);
	void sendValidate();

	FGameServerFlow* _gameServer = nullptr;
	uint32 _localObjectId = 0;

	FVector _location = FVector::ZeroVector;
	FVector _previousLocation = FVector::ZeroVector;
	FVector _moveDirection = FVector::ZeroVector;
	float _yaw = 0.0f;
	bool _wantsToMove = false;
	bool _hasLocalState = false;
	bool _moving = false;

	float _sinceMoveSent = 0.0f;
	float _sinceValidateSent = 0.0f;
};
}
