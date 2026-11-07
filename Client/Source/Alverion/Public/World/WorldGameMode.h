#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameServer/GameServerPackets.h"
#include "GameServer/WorldFlow.h"

#include "WorldGameMode.generated.h"

class AProxyCharacter;
class URootLayout;
class UServerConnectionSubsystem;

/** The continent's game mode: the player's character where the server put it, and everybody the server says is around. */
UCLASS()
class ALVERION_API AWorldGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AWorldGameMode();

	/** Takes the character out of the world for the character list, keeping the link, and travels to it once the server agrees. */
	void returnToCharacterSelect();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

private:
	void spawned(const Network::FCharacterSpawn& spawn);
	void despawned(uint32 objectId);
	void moved(const Network::FObjectPlace& place, Network::EMoveKind kind);
	UServerConnectionSubsystem* connection() const;

	UPROPERTY()
	TMap<uint32, TObjectPtr<AProxyCharacter>> _proxies;

	UPROPERTY()
	TObjectPtr<URootLayout> _rootLayout;
};
