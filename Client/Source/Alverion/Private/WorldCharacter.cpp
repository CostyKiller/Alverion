#include "WorldCharacter.h"

#include "AppearanceApplier.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WorldPartitionStreamingSourceComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "GameSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
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
	constexpr double settleSearchCm = 20000.0;

	const FLinearColor ownBodyColor(0.2f, 0.45f, 0.9f);

	// Speeds in centimetres a second below which the body stands still, and below which it walks rather than runs.
	constexpr double standingSpeedCm = 10.0;
	constexpr double walkingSpeedCm = 250.0;

	// What the options screen's sensitivity reads on a fresh install, which is the turn rate the mouse had before the slider existed.
	constexpr float defaultMouseSensitivity = 0.3f;

	const TCHAR* const moveForwardPath = TEXT("/Game/Input/Actions/IA_MoveForward.IA_MoveForward");
	const TCHAR* const moveBackwardPath = TEXT("/Game/Input/Actions/IA_MoveBackward.IA_MoveBackward");
	const TCHAR* const moveLeftPath = TEXT("/Game/Input/Actions/IA_MoveLeft.IA_MoveLeft");
	const TCHAR* const moveRightPath = TEXT("/Game/Input/Actions/IA_MoveRight.IA_MoveRight");
	const TCHAR* const jumpPath = TEXT("/Game/Input/Actions/IA_Jump.IA_Jump");

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

	SceneSetup::paintPlaceholderBody(*_body, ownBodyColor);

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

	if (_metaHuman != nullptr)
	{
		_metaHuman->Destroy();
		_metaHuman = nullptr;
	}

	Super::EndPlay(reason);
}

void AWorldCharacter::wear(const Network::FAppearance& appearance)
{
	// The actor stands at the middle of its capsule, so the feet are half its height below.
	const TWeakObjectPtr<AWorldCharacter> weakThis(this);
	Appearance::attachTo(*this, appearance, FVector(0.0, 0.0, -capsuleHalfHeightCm), [weakThis](AActor* body)
	{
		AWorldCharacter* character = weakThis.Get();
		if ((character == nullptr) || (body == nullptr))
		{
			return;
		}

		character->_metaHuman = body;
		character->_body->SetVisibility(false);
	});
}

void AWorldCharacter::PawnClientRestart()
{
	// Before the parent's restart, because that is where SetupPlayerInputComponent binds them.
	// The movement actions are the rebindable assets the options screen lists. Look stays a raw mouse action made here, since no look action is rebindable.
	if (_lookMapping == nullptr)
	{
		_moveForwardAction = LoadObject<UInputAction>(nullptr, moveForwardPath);
		_moveBackwardAction = LoadObject<UInputAction>(nullptr, moveBackwardPath);
		_moveLeftAction = LoadObject<UInputAction>(nullptr, moveLeftPath);
		_moveRightAction = LoadObject<UInputAction>(nullptr, moveRightPath);
		_jumpAction = LoadObject<UInputAction>(nullptr, jumpPath);

		_lookAction = NewObject<UInputAction>(this);
		_lookAction->ValueType = EInputActionValueType::Axis2D;

		_lookMapping = NewObject<UInputMappingContext>(this);
		_lookMapping->MapKey(_lookAction, EKeys::Mouse2D);
	}

	Super::PawnClientRestart();

	// Added beside the bootstrap's context rather than in place of it, which the controller applies once per level.
	APlayerController* controller = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* input = (controller != nullptr) ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(controller->GetLocalPlayer()) : nullptr;
	if (input != nullptr)
	{
		input->AddMappingContext(_lookMapping, 1);
	}
}

void AWorldCharacter::SetupPlayerInputComponent(UInputComponent* input)
{
	Super::SetupPlayerInputComponent(input);

	UEnhancedInputComponent* enhanced = Cast<UEnhancedInputComponent>(input);
	if (enhanced == nullptr)
	{
		return;
	}

	if ((_moveForwardAction == nullptr) || (_moveBackwardAction == nullptr) || (_moveLeftAction == nullptr) || (_moveRightAction == nullptr))
	{
		UE_LOG(LogTemp, Warning, TEXT("WorldCharacter is missing a movement action asset, so some movement keys do nothing."));
	}

	if (_moveForwardAction != nullptr)
	{
		enhanced->BindAction(_moveForwardAction, ETriggerEvent::Triggered, this, &AWorldCharacter::moveForward);
	}

	if (_moveBackwardAction != nullptr)
	{
		enhanced->BindAction(_moveBackwardAction, ETriggerEvent::Triggered, this, &AWorldCharacter::moveBackward);
	}

	if (_moveLeftAction != nullptr)
	{
		enhanced->BindAction(_moveLeftAction, ETriggerEvent::Triggered, this, &AWorldCharacter::moveLeft);
	}

	if (_moveRightAction != nullptr)
	{
		enhanced->BindAction(_moveRightAction, ETriggerEvent::Triggered, this, &AWorldCharacter::moveRight);
	}

	if (_jumpAction != nullptr)
	{
		enhanced->BindAction(_jumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		enhanced->BindAction(_jumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}

	if (_lookAction != nullptr)
	{
		enhanced->BindAction(_lookAction, ETriggerEvent::Triggered, this, &AWorldCharacter::look);
	}
}

void AWorldCharacter::moveForward()
{
	move(1.0f, 0.0f);
}

void AWorldCharacter::moveBackward()
{
	move(-1.0f, 0.0f);
}

void AWorldCharacter::moveLeft()
{
	move(0.0f, -1.0f);
}

void AWorldCharacter::moveRight()
{
	move(0.0f, 1.0f);
}

void AWorldCharacter::move(float forward, float right)
{
	// Yaw only, so looking up does not walk the character into the ground.
	const FRotator facing(0.0f, GetControlRotation().Yaw, 0.0f);

	AddMovementInput(FRotationMatrix(facing).GetUnitAxis(EAxis::X), forward);
	AddMovementInput(FRotationMatrix(facing).GetUnitAxis(EAxis::Y), right);
	_movedThisFrame = true;
}

void AWorldCharacter::look(const FInputActionValue& value)
{
	// Measured against the default sensitivity, so a fresh install turns exactly as it always did and the slider scales from there.
	const UGameSettings* settings = UGameSettings::get();
	const float scale = (settings != nullptr) ? (settings->getMouseSensitivity() / defaultMouseSensitivity) : 1.0f;

	const FVector2D axes = value.Get<FVector2D>();
	AddControllerYawInput(axes.X * scale);
	AddControllerPitchInput(-axes.Y * scale);
}

void AWorldCharacter::Tick(float deltaSeconds)
{
	Super::Tick(deltaSeconds);

	if (!_onGround)
	{
		_onGround = settleOnGround();
	}

	// Read every frame, so a change made in the options screen over the world shows at once.
	const UGameSettings* settings = UGameSettings::get();
	if ((settings != nullptr) && (_camera->FieldOfView != settings->getFieldOfView()))
	{
		_camera->SetFieldOfView(settings->getFieldOfView());
	}

	// The worn body plays what the capsule is doing: in the air, standing, walking or running.
	if (_metaHuman != nullptr)
	{
		const double speed = GetVelocity().Size2D();
		Appearance::EMotion motion = Appearance::EMotion::Run;
		if (GetCharacterMovement()->IsFalling())
		{
			motion = Appearance::EMotion::Fall;
		}
		else if (speed < standingSpeedCm)
		{
			motion = Appearance::EMotion::Idle;
		}
		else if (speed < walkingSpeedCm)
		{
			motion = Appearance::EMotion::Walk;
		}

		Appearance::animate(*_metaHuman, motion);
	}

	// The server places a character by its feet, and the actor stands at the middle of its capsule.
	// The movement bindings fire before this tick, so the flag says whether a key was held this frame.
	UServerConnectionSubsystem* connection = connectionOf(*this);
	if (connection != nullptr)
	{
		const FVector feet = GetActorLocation() - FVector(0.0f, 0.0f, capsuleHalfHeightCm);
		connection->world().setLocalState(feet, GetActorRotation().Yaw, _movedThisFrame);
	}

	_movedThisFrame = false;
}

bool AWorldCharacter::settleOnGround()
{
	const FVector here = GetActorLocation();
	FCollisionQueryParams query;
	query.AddIgnoredActor(this);

	FHitResult hit;
	if (!GetWorld()->LineTraceSingleByChannel(hit, here + FVector(0.0, 0.0, settleSearchCm), here - FVector(0.0, 0.0, settleSearchCm), ECC_WorldStatic, query))
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
