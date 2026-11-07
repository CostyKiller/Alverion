#pragma once

#include "CoreMinimal.h"
#include "GameServer/GameServerFlow.h"
#include "LoginServer/LoginServerFlow.h"
#include "Session.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GameServer/WorldFlow.h"

#include "ServerConnectionSubsystem.generated.h"

/** Both server links and the flows driving them, kept for the life of the game so level travel never drops a connection. */
UCLASS()
class NETWORK_API UServerConnectionSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	/** Opens the login server link and starts logging in. Progress and failure arrive through loginServer()'s delegates. */
	bool startLogin(const FString& host, int32 port, const FString& account, const FString& password);

	/** Closes the login server link and opens the game server named in its token. */
	bool startGameServer();

	/** Closes both links and ends every flow. */
	void disconnectAll();

	Network::FLoginServerFlow& loginServer();
	Network::FGameServerFlow& gameServer();
	Network::FWorldFlow& world();

	virtual void Tick(float deltaSeconds) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

private:
	Network::FSession _loginSession;
	Network::FSession _gameSession;
	Network::FLoginServerFlow _loginServer;
	Network::FGameServerFlow _gameServer;
	Network::FWorldFlow _world;
};
