#include "GameServer/WorldFlow.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlverionWorld, Display, All);

namespace Network
{
namespace
{
	// How far ahead of the character the keyboard destination goes. Far enough that the server never sees it arrive between reports.
	constexpr double moveLookaheadCm = 600.0;

	// A held key is reported again this often, and at once when the direction swings by more than about twenty degrees.
	constexpr float moveResendSeconds = 0.5f;
	constexpr double moveResendCosine = 0.94;

	// How often the position goes out while moving. The server takes it as true in keyboard mode.
	constexpr float validateSeconds = 0.25f;

	// Slower than this is standing still, so a character settling onto the ground does not stream moves.
	constexpr double movingSpeedCm = 20.0;

	// A correction smaller than this is the server repeating where this client already said it was.
	constexpr double correctionToleranceCm = 150.0;
}

FWorldFlow::~FWorldFlow()
{
	end();
}

bool FWorldFlow::begin(FGameServerFlow& gameServer)
{
	_gameServer = &gameServer;
	_localObjectId = gameServer.selected().objectId;
	_hasLocalState = false;
	_moving = false;
	_sinceMoveSent = 0.0f;
	_sinceValidateSent = 0.0f;

	_gameServer->onWorldPacket = FPacketHandler::CreateRaw(this, &FWorldFlow::handlePacket);
	return _gameServer->enterWorld();
}

void FWorldFlow::end()
{
	if (_gameServer != nullptr)
	{
		_gameServer->onWorldPacket = FPacketHandler();
		_gameServer = nullptr;
	}
}

bool FWorldFlow::isActive() const
{
	return _gameServer != nullptr;
}

void FWorldFlow::setLocalState(const FVector& location, float yaw, bool wantsToMove)
{
	_location = location;
	_yaw = yaw;
	_wantsToMove = wantsToMove;

	if (!_hasLocalState)
	{
		_previousLocation = location;
		_hasLocalState = true;
	}
}

void FWorldFlow::tick(float deltaSeconds)
{
	if ((_gameServer == nullptr) || !_hasLocalState || (deltaSeconds <= 0.0f))
	{
		return;
	}

	_sinceMoveSent += deltaSeconds;
	_sinceValidateSent += deltaSeconds;

	// The direction comes from how the character actually moved, so it agrees with whatever the camera made of the keys.
	const FVector step = _location - _previousLocation;
	_previousLocation = _location;

	const double speed = step.Size2D() / static_cast<double>(deltaSeconds);
	if (_wantsToMove && (speed > movingSpeedCm))
	{
		const FVector direction = step.GetSafeNormal2D();
		const bool turned = (FVector::DotProduct(direction, _moveDirection) < moveResendCosine);

		if (!_moving || turned || (_sinceMoveSent >= moveResendSeconds))
		{
			_moveDirection = direction;
			sendMove(_location + (direction * moveLookaheadCm));
		}

		if (_sinceValidateSent >= validateSeconds)
		{
			sendValidate();
		}

		_moving = true;
		return;
	}

	if (_moving)
	{
		// A destination equal to the origin is how a client says stop.
		sendMove(_location);
		sendValidate();
		_moving = false;
	}
}

void FWorldFlow::sendMove(const FVector& target)
{
	FWriteBuffer buffer;
	GameServer::writeMoveToLocation(buffer, target, _location);
	_gameServer->session()->send(buffer.bytes());
	_sinceMoveSent = 0.0f;
}

void FWorldFlow::sendValidate()
{
	FWriteBuffer buffer;
	GameServer::writeValidatePosition(buffer, _location, _yaw);
	_gameServer->session()->send(buffer.bytes());
	_sinceValidateSent = 0.0f;
}

void FWorldFlow::handlePacket(uint32 opcode, TArrayView<const uint8> body)
{
	FReadBuffer buffer(body);

	switch (opcode)
	{
		case GameServer::Opcode::userInfo:
		{
			uint32 objectId = 0;
			if (GameServer::readUserInfoObjectId(buffer, objectId))
			{
				_localObjectId = objectId;
			}

			return;
		}
		case GameServer::Opcode::charInfo:
		{
			FCharacterSpawn spawn;
			if (GameServer::readCharInfo(body, spawn) && (spawn.objectId != _localObjectId))
			{
				onSpawned.ExecuteIfBound(spawn);
			}

			return;
		}
		case GameServer::Opcode::npcInfo:
		{
			FCharacterSpawn spawn;
			if (GameServer::readNpcInfo(buffer, spawn))
			{
				onSpawned.ExecuteIfBound(spawn);
			}
			else
			{
				UE_LOG(LogAlverionWorld, Warning, TEXT("NpcInfo of %d bytes did not parse."), body.Num());
			}

			return;
		}
		case GameServer::Opcode::deleteObject:
		{
			int32 objectId = 0;
			if (buffer.readInt32(objectId))
			{
				onDespawned.ExecuteIfBound(static_cast<uint32>(objectId));
			}

			return;
		}
		case GameServer::Opcode::moveToLocation:
		{
			// This client's own moves come back to it as well, and it is already where it says it is going.
			FObjectPlace destination;
			if (GameServer::readObjectPlace(buffer, destination) && (destination.objectId != _localObjectId))
			{
				onMoved.ExecuteIfBound(destination, EMoveKind::Destination);
			}

			return;
		}
		case GameServer::Opcode::stopMove:
		case GameServer::Opcode::validateLocation:
		case GameServer::Opcode::teleportToLocation:
		{
			const bool teleport = (opcode == GameServer::Opcode::teleportToLocation);

			FObjectPlace place;
			if (!GameServer::readObjectPlaceWithHeading(buffer, place, teleport))
			{
				return;
			}

			if (place.objectId != _localObjectId)
			{
				onMoved.ExecuteIfBound(place, EMoveKind::Stopped);
				return;
			}

			if (teleport)
			{
				onCorrected.ExecuteIfBound(place, true);

				// The jump is not a run, so speed is measured again from wherever the character lands.
				_hasLocalState = false;
				_moving = false;

				FWriteBuffer reply;
				GameServer::writeAppearing(reply);
				_gameServer->session()->send(reply.bytes());
				return;
			}

			// Keyboard mode makes this client the authority, so the server echoing its own position is not a correction.
			if (FVector::Dist2D(place.location, _location) > correctionToleranceCm)
			{
				onCorrected.ExecuteIfBound(place, false);
			}

			return;
		}
		default:
		{
			UE_LOG(LogAlverionWorld, Verbose, TEXT("Ignored world packet 0x%X, %d bytes."), opcode, body.Num());
			return;
		}
	}
}
}
