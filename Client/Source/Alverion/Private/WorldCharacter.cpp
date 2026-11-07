#include "WorldCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "SceneSetup.h"
#include "ServerConnectionSubsystem.h"

namespace
{
	// A starting character's 115 units a second at the client's scale. The server takes this client's word for where it is, so the two need not match exactly.
	constexpr float runSpeedCm = 460.0f;

	constexpr float capsuleRadiusCm = 34.0f;
	constexpr float capsuleHalfHeightCm = 90.0f;
	constexpr float cameraDistanceCm = 450.0f;

	// How far above and below the server's height to look for ground, which is shaped by hand and need not match it.
	constexpr double groundSearchCm = 20000.0;

	const FLinearColor playerColor(0.2f, 0.45f, 0.9f);

	UServerConnectionSubsystem* connectionOf(const AActor& actor)
	{
		UGameInstance* instance = actor.GetGameInstance();
		return (instance != nullptr) ? instance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
	}
}

AWorldCharacter::AWorldCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(capsuleRadiusCm, capsuleHalfHeightCm);

	// The body faces where it walks and the camera turns freely around it.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = runSpeedCm;

	_arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Arm"));
	_arm->SetupAttachment(RootComponent);
	_arm->TargetArmLength = cameraDistanceCm;
	_arm->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	_arm->bUsePawnControlRotation = true;

	_camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	_camera->SetupAttachment(_arm, USpringArmComponent::SocketName);

	_body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	_body->SetupAttachment(RootComponent);

	_streaming = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("Streaming"));
}

void AWorldCharacter::BeginPlay()
{
	Super::BeginPlay();

	SceneSetup::paintPlaceholderBody(*_body, playerColor);

	// Held in the air until the ground streams in, or it would fall through the sea before the first section loads.
	GetCharacterMovement()->DisableMovement();

	UServerConnectionSubsystem* connection = connectionOf(*this);
	if (connection != nullptr)
	{
		connection->world().onCorrected.BindUObject(this, &AWorldCharacter::corrected);
	}
}

void AWorldCharacter::EndPlay(const EEndPlayReason::Type reason)
{
	UServerConnectionSubsystem* connection = connectionOf(*this);
	if (connection != nullptr)
	{
		connection->world().onCorrected.Unbind();
	}

	Super::EndPlay(reason);
}

void AWorldCharacter::PawnClientRestart()
{
	// Made here rather than as assets, since the project keeps no input assets: forward and back are the swizzled vertical axis, left is negated.
	// Before the parent's restart, because that is where SetupPlayerInputComponent binds them.
	if (_mapping == nullptr)
	{
		_moveAction = NewObject<UInputAction>(this);
		_moveAction->ValueType = EInputActionValueType::Axis2D;

		_lookAction = NewObject<UInputAction>(this);
		_lookAction->ValueType = EInputActionValueType::Axis2D;

		_mapping = NewObject<UInputMappingContext>(this);

		FEnhancedActionKeyMapping& forward = _mapping->MapKey(_moveAction, EKeys::W);
		forward.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(_mapping));

		FEnhancedActionKeyMapping& back = _mapping->MapKey(_moveAction, EKeys::S);
		back.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(_mapping));
		back.Modifiers.Add(NewObject<UInputModifierNegate>(_mapping));

		FEnhancedActionKeyMapping& left = _mapping->MapKey(_moveAction, EKeys::A);
		left.Modifiers.Add(NewObject<UInputModifierNegate>(_mapping));

		_mapping->MapKey(_moveAction, EKeys::D);
		_mapping->MapKey(_lookAction, EKeys::Mouse2D);
	}

	Super::PawnClientRestart();

	APlayerController* controller = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* input = (controller != nullptr) ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(controller->GetLocalPlayer()) : nullptr;
	if (input != nullptr)
	{
		input->ClearAllMappings();
		input->AddMappingContext(_mapping, 0);
	}
}

void AWorldCharacter::SetupPlayerInputComponent(UInputComponent* input)
{
	Super::SetupPlayerInputComponent(input);

	UEnhancedInputComponent* enhanced = Cast<UEnhancedInputComponent>(input);
	if ((enhanced == nullptr) || (_moveAction == nullptr))
	{
		return;
	}

	enhanced->BindAction(_moveAction, ETriggerEvent::Triggered, this, &AWorldCharacter::move);
	enhanced->BindAction(_moveAction, ETriggerEvent::Completed, this, &AWorldCharacter::stopMoving);
	enhanced->BindAction(_lookAction, ETriggerEvent::Triggered, this, &AWorldCharacter::look);
}

void AWorldCharacter::move(const FInputActionValue& value)
{
	const FVector2D axes = value.Get<FVector2D>();
	const FRotator facing(0.0f, GetControlRotation().Yaw, 0.0f);

	AddMovementInput(FRotationMatrix(facing).GetUnitAxis(EAxis::X), axes.Y);
	AddMovementInput(FRotationMatrix(facing).GetUnitAxis(EAxis::Y), axes.X);
	_wantsToMove = true;
}

void AWorldCharacter::stopMoving(const FInputActionValue& value)
{
	_wantsToMove = false;
}

void AWorldCharacter::look(const FInputActionValue& value)
{
	const FVector2D axes = value.Get<FVector2D>();
	AddControllerYawInput(axes.X);
	AddControllerPitchInput(-axes.Y);
}

void AWorldCharacter::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (!_onGround)
	{
		_onGround = settleOnGround();
	}

	// The server places a character by its feet, and the actor stands at the middle of its capsule.
	UServerConnectionSubsystem* connection = connectionOf(*this);
	if (connection != nullptr)
	{
		const FVector feet = GetActorLocation() - FVector(0.0f, 0.0f, capsuleHalfHeightCm);
		connection->world().setLocalState(feet, GetActorRotation().Yaw, _wantsToMove);
	}
}

bool AWorldCharacter::settleOnGround()
{
	const FVector here = GetActorLocation();
	FCollisionQueryParams query;
	query.AddIgnoredActor(this);

	FHitResult hit;
	if (!GetWorld()->LineTraceSingleByChannel(hit, here + FVector(0.0, 0.0, groundSearchCm), here - FVector(0.0, 0.0, groundSearchCm), ECC_WorldStatic, query))
	{
		return false;
	}

	SetActorLocation(hit.ImpactPoint + FVector(0.0, 0.0, capsuleHalfHeightCm + 2.0), false, nullptr, ETeleportType::TeleportPhysics);
	GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	return true;
}

void AWorldCharacter::corrected(const Network::FObjectPlace& place, bool teleport)
{
	SetActorLocation(place.location + FVector(0.0f, 0.0f, capsuleHalfHeightCm), false, nullptr, teleport ? ETeleportType::TeleportPhysics : ETeleportType::None);

	if (teleport)
	{
		SetActorRotation(FRotator(0.0f, place.yaw, 0.0f));
	}
}
