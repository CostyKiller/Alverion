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

	/** Stands the MetaHuman this appearance describes on the character, in place of the placeholder figure. */
	void wear(const Network::FAppearance& appearance);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type reason) override;

private:
	void moveForward();
	void moveBackward();
	void moveLeft();
	void moveRight();
	void move(float forward, float right);
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

	/** The worn MetaHuman, riding on the capsule. Destroyed with the character, since an attached actor outlives its parent. */
	UPROPERTY()
	TObjectPtr<AActor> _metaHuman;

	/** The continent streams around the player, and loads nothing without a source. */
	UPROPERTY()
	TObjectPtr<UWorldPartitionStreamingSourceComponent> _streaming;

	/** Mouse look only. The movement keys live in the rebindable context the input bootstrap builds from the action assets. */
	UPROPERTY()
	TObjectPtr<UInputMappingContext> _lookMapping;

	UPROPERTY()
	TObjectPtr<UInputAction> _lookAction;

	UPROPERTY()
	TObjectPtr<UInputAction> _moveForwardAction;

	UPROPERTY()
	TObjectPtr<UInputAction> _moveBackwardAction;

	UPROPERTY()
	TObjectPtr<UInputAction> _moveLeftAction;

	UPROPERTY()
	TObjectPtr<UInputAction> _moveRightAction;

	UPROPERTY()
	TObjectPtr<UInputAction> _jumpAction;

	/** Set by any movement key this frame, read and cleared by Tick: four keys can be held at once, so no single release means stopped. */
	bool _movedThisFrame = false;
	bool _onGround = false;
};
