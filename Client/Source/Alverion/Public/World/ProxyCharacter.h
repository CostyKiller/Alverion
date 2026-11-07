#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameServer/GameServerPackets.h"

#include "ProxyCharacter.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;

/**
 * Somebody or something else in the world: another player, a townsperson or a monster.
 * The server sends a destination and lets the client walk there, so this walks at the mover's own speed and keeps its feet on the island.
 */
UCLASS()
class ALVERION_API AProxyCharacter : public AActor
{
	GENERATED_BODY()

public:
	AProxyCharacter();

	void setup(const Network::FCharacterSpawn& spawn);
	void walkTo(const FVector& destination);
	void standAt(const FVector& location, float yaw);

	virtual void Tick(float deltaSeconds) override;

protected:
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

private:
	/** The island's surface under a place, or the place itself where there is none. */
	FVector onGround(const FVector& location) const;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> _body;

	UPROPERTY()
	TObjectPtr<UTextRenderComponent> _label;

	/** The MetaHuman a player wears, riding on this actor. Townspeople and monsters have no appearance and keep the placeholder. */
	UPROPERTY()
	TObjectPtr<AActor> _metaHuman;

	/** The bytes the MetaHuman was dressed from, so a repeated spawn does not stand up a second body. */
	TArray<uint8> _wornAppearance;

	FVector _destination = FVector::ZeroVector;
	float _speed = 0.0f;
	bool _walking = false;
};
