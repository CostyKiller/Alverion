#include "ServerConnectionSubsystem.h"

namespace
{
	// A busy area sends a burst of spawns on arrival. This many a frame drains it in a second without stalling one.
	constexpr int32 packetsPerTick = 256;
}

void UServerConnectionSubsystem::Deinitialize()
{
	disconnectAll();
	Super::Deinitialize();
}

bool UServerConnectionSubsystem::startLogin(const FString& host, int32 port, const FString& account, const FString& password)
{
	disconnectAll();

	if (!_loginSession.connect(MakeUnique<Network::FTcpTransport>(), host, port, MakeUnique<Network::FLoginCipher>()))
	{
		return false;
	}

	_loginServer.begin(_loginSession, account, password);
	return true;
}

bool UServerConnectionSubsystem::startGameServer()
{
	if (_loginServer.step() != Network::ELoginServerStep::TokenIssued)
	{
		return false;
	}

	const Network::FLoginToken token = _loginServer.token();
	_loginServer.end();
	_loginSession.disconnect();

	if (!_gameSession.connect(MakeUnique<Network::FTcpTransport>(), token.host, token.port, MakeUnique<Network::FGameCipher>()))
	{
		return false;
	}

	_gameServer.begin(_gameSession, token);
	return true;
}

void UServerConnectionSubsystem::disconnectAll()
{
	_world.end();
	_gameServer.end();
	_loginServer.end();
	_gameSession.disconnect();
	_loginSession.disconnect();
}

Network::FLoginServerFlow& UServerConnectionSubsystem::loginServer()
{
	return _loginServer;
}

Network::FGameServerFlow& UServerConnectionSubsystem::gameServer()
{
	return _gameServer;
}

Network::FWorldFlow& UServerConnectionSubsystem::world()
{
	return _world;
}

void UServerConnectionSubsystem::Tick(float deltaSeconds)
{
	_loginSession.pump(packetsPerTick);
	_gameSession.pump(packetsPerTick);

	// After the pumps, so a step the packets completed is acted on this frame.
	_loginServer.tick();
	_gameServer.tick();
	_world.tick(deltaSeconds);
}

TStatId UServerConnectionSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UServerConnectionSubsystem, STATGROUP_Tickables);
}

bool UServerConnectionSubsystem::IsTickable() const
{
	return !IsTemplate() && ((_loginSession.state() != Network::ESessionState::Disconnected) || (_gameSession.state() != Network::ESessionState::Disconnected));
}
