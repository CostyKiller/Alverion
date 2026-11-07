#include "WorldGameMode.h"

#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ProxyCharacter.h"
#include "ServerConnectionSubsystem.h"
#include "WorldCharacter.h"

namespace
{
	// Half the character's capsule, plus a little so it starts above the server's height rather than inside the ground.
	constexpr double spawnLiftCm = 100.0;
}

AWorldGameMode::AWorldGameMode()
{
	DefaultPawnClass = nullptr;
}

UServerConnectionSubsystem* AWorldGameMode::connection() const
{
	UGameInstance* instance = GetGameInstance();
	return (instance != nullptr) ? instance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
}

void AWorldGameMode::BeginPlay()
{
	Super::BeginPlay();

	UWorld* world = GetWorld();
	UServerConnectionSubsystem* link = connection();
	if ((world == nullptr) || (link == nullptr))
	{
		return;
	}

	FActorSpawnParameters parameters;
	parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// The map's ground streams in after this, so the character waits where the server put it until there is ground under it.
	const Network::FCharacterSelected& selected = link->gameServer().selected();
	AWorldCharacter* character = world->SpawnActor<AWorldCharacter>(selected.location + FVector(0.0, 0.0, spawnLiftCm), FRotator::ZeroRotator, parameters);

	APlayerController* controller = UGameplayStatics::GetPlayerController(this, 0);
	if ((controller != nullptr) && (character != nullptr))
	{
		controller->Possess(character);
		controller->SetShowMouseCursor(false);
		controller->SetInputMode(FInputModeGameOnly());
	}

	Network::FWorldFlow& flow = link->world();
	flow.onSpawned.BindUObject(this, &AWorldGameMode::spawned);
	flow.onDespawned.BindUObject(this, &AWorldGameMode::despawned);
	flow.onMoved.BindUObject(this, &AWorldGameMode::moved);

	// A lost link goes back to the login screen, which is what the front end shows for anything but a character list.
	link->gameServer().onFailed.BindWeakLambda(this, [this](const FString& reason)
	{
		UGameplayStatics::OpenLevel(this, FName(TEXT("/Engine/Maps/Entry")));
	});

	flow.begin(link->gameServer());
}

void AWorldGameMode::EndPlay(const EEndPlayReason::Type reason)
{
	UServerConnectionSubsystem* link = connection();
	if (link != nullptr)
	{
		Network::FWorldFlow& flow = link->world();
		flow.onSpawned.Unbind();
		flow.onDespawned.Unbind();
		flow.onMoved.Unbind();
		flow.end();
		link->gameServer().onFailed.Unbind();
	}

	Super::EndPlay(reason);
}

void AWorldGameMode::spawned(const Network::FCharacterSpawn& spawn)
{
	// A spawn for somebody already here is the server repeating itself, so the existing one is moved and dressed again.
	TObjectPtr<AProxyCharacter>* existing = _proxies.Find(spawn.objectId);
	if ((existing != nullptr) && (*existing != nullptr))
	{
		(*existing)->setup(spawn);
		return;
	}

	FActorSpawnParameters parameters;
	parameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProxyCharacter* proxy = GetWorld()->SpawnActor<AProxyCharacter>(spawn.location, FRotator::ZeroRotator, parameters);
	if (proxy != nullptr)
	{
		proxy->setup(spawn);
		_proxies.Add(spawn.objectId, proxy);
	}
}

void AWorldGameMode::despawned(uint32 objectId)
{
	TObjectPtr<AProxyCharacter> proxy;
	if (_proxies.RemoveAndCopyValue(objectId, proxy) && (proxy != nullptr))
	{
		proxy->Destroy();
	}
}

void AWorldGameMode::moved(const Network::FObjectPlace& place, Network::EMoveKind kind)
{
	TObjectPtr<AProxyCharacter>* proxy = _proxies.Find(place.objectId);
	if ((proxy == nullptr) || (*proxy == nullptr))
	{
		return;
	}

	if (kind == Network::EMoveKind::Destination)
	{
		(*proxy)->walkTo(place.location);
	}
	else
	{
		(*proxy)->standAt(place.location, place.yaw);
	}
}
