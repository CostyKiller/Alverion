#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameServerPackets.h"

#include "WorldCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
class UWorldPartitionStreamingSourceComponent;
struct FInputActionValue;

/** The player's own character. It moves itself and tells the server where it is, which the server accepts in keyboard mode. */
UCLASS()
class ALVERION_API AWorldCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AWorldCharacter();

	virtual void Tick(float deltaSeconds) override;
	virtual void PawnClientRestart() override;
	virtual void SetupPlayerInputComponent(UInputComponent* input) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

private:
	void move(const FInputActionValue& value);
	void stopMoving(const FInputActionValue& value);
	void look(const FInputActionValue& value);
	void corrected(const Network::FObjectPlace& place, bool teleport);

	/** Puts the character on the ground under it and lets it walk, once streamed ground is there. */
	bool settleOnGround();

	UPROPERTY()
	TObjectPtr<USpringArmComponent> _arm;

	UPROPERTY()
	TObjectPtr<UCameraComponent> _camera;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> _body;

	/** The continent streams around the player, and loads nothing without a source. */
	UPROPERTY()
	TObjectPtr<UWorldPartitionStreamingSourceComponent> _streaming;

	UPROPERTY()
	TObjectPtr<UInputMappingContext> _mapping;

	UPROPERTY()
	TObjectPtr<UInputAction> _moveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> _lookAction;

	bool _wantsToMove = false;
	bool _onGround = false;
};
