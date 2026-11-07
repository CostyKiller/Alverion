#include "FrontEndGameMode.h"

#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "FrontEndScreens.h"
#include "Kismet/GameplayStatics.h"
#include "SceneSetup.h"
#include "ServerConnectionSubsystem.h"

#define LOCTEXT_NAMESPACE "AlverionFrontEnd"

namespace
{
	constexpr int32 defaultLoginPort = 2106;

	const TCHAR* const continentMap = TEXT("/Game/Maps/L_Continent");
}

AFrontEndGameMode::AFrontEndGameMode()
{
	DefaultPawnClass = nullptr;
}

void AFrontEndGameMode::travelToWorld(UObject* worldContext)
{
	UGameplayStatics::OpenLevel(worldContext, FName(continentMap), true, TEXT("game=/Script/Alverion.WorldGameMode"));
}

void AFrontEndGameMode::BeginPlay()
{
	Super::BeginPlay();

	SceneSetup::buildDaylight(*GetWorld());

	APlayerController* controller = UGameplayStatics::GetPlayerController(this, 0);
	if (controller != nullptr)
	{
		controller->SetShowMouseCursor(true);
		controller->SetInputMode(FInputModeUIOnly());
	}

	bindFlows();

	// Back from the world, or from a lost link: the screen that matches where the game server conversation stands.
	UServerConnectionSubsystem* link = connection();
	if ((link != nullptr) && (link->gameServer().step() == Network::EGameServerStep::CharacterSelect))
	{
		showCharacters();
		return;
	}

	showLogin();

#if !UE_BUILD_SHIPPING
	// For testing without a hand on the keyboard. Shipping builds have no such door.
	FString autoLogin;
	TArray<FString> parts;
	if (FParse::Value(FCommandLine::Get(), TEXT("autologin="), autoLogin, false) && (autoLogin.ParseIntoArray(parts, TEXT(",")) >= 2))
	{
		_autoEnter = parts.IsValidIndex(2) ? parts[2] : FString();
		login(TEXT("127.0.0.1"), parts[0], parts[1]);
	}
#endif
}

void AFrontEndGameMode::EndPlay(const EEndPlayReason::Type reason)
{
	unbindFlows();

	if (_shown.IsValid() && (GEngine != nullptr) && (GEngine->GameViewport != nullptr))
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(_shown.ToSharedRef());
	}

	Super::EndPlay(reason);
}

UServerConnectionSubsystem* AFrontEndGameMode::connection() const
{
	UGameInstance* instance = GetGameInstance();
	return (instance != nullptr) ? instance->GetSubsystem<UServerConnectionSubsystem>() : nullptr;
}

void AFrontEndGameMode::show(const TSharedRef<SWidget>& screen)
{
	UGameViewportClient* viewport = (GEngine != nullptr) ? GEngine->GameViewport : nullptr;
	if (viewport == nullptr)
	{
		return;
	}

	if (_shown.IsValid())
	{
		viewport->RemoveViewportWidgetContent(_shown.ToSharedRef());
	}

	_shown = screen;
	viewport->AddViewportWidgetContent(screen);
}

void AFrontEndGameMode::showLogin()
{
	_login = SNew(SLoginScreen).OnLogin(FOnLoginRequested::CreateUObject(this, &AFrontEndGameMode::login));
	show(_login.ToSharedRef());
}

void AFrontEndGameMode::showCharacters()
{
	UServerConnectionSubsystem* link = connection();
	if (link == nullptr)
	{
		return;
	}

	_characters = SNew(SCharacterScreen)
		.Characters(link->gameServer().characters())
		.OnEnter_Lambda([this](uint32 objectId)
		{
			if (!connection()->gameServer().selectCharacter(objectId))
			{
				_characters->setStatus(LOCTEXT("CannotEnter", "That character cannot be entered now."));
			}
		})
		.OnDelete_Lambda([this](uint32 objectId)
		{
			connection()->gameServer().deleteCharacter(objectId);
		})
		.OnCreate_Lambda([this]()
		{
			showCreate();
		})
		.OnBack_Lambda([this]()
		{
			connection()->disconnectAll();
			showLogin();
		});

	show(_characters.ToSharedRef());
}

void AFrontEndGameMode::showCreate()
{
	_create = SNew(SCreateScreen)
		.OnCreate_Lambda([this](const FString& name, const Network::FAppearance& appearance)
		{
			if (!connection()->gameServer().createCharacter(name, appearance))
			{
				_create->setStatus(LOCTEXT("CannotCreate", "Not connected to a game server."));
			}
		})
		.OnBack_Lambda([this]()
		{
			showCharacters();
		});

	show(_create.ToSharedRef());
}

void AFrontEndGameMode::login(const FString& host, const FString& account, const FString& password)
{
	UServerConnectionSubsystem* link = connection();
	if ((link == nullptr) || account.IsEmpty() || password.IsEmpty())
	{
		_login->setStatus(LOCTEXT("Missing", "Enter an account and a password."));
		return;
	}

	FString address = host;
	FString portText;
	int32 port = defaultLoginPort;
	if (host.Split(TEXT(":"), &address, &portText))
	{
		port = FCString::Atoi(*portText);
	}

	_login->setBusy(true);
	_login->setStatus(LOCTEXT("Connecting", "Connecting..."));

	if (!link->startLogin(address, port, account, password))
	{
		_login->setBusy(false);
		_login->setStatus(LOCTEXT("NoConnect", "Could not connect."));
	}
}

void AFrontEndGameMode::bindFlows()
{
	UServerConnectionSubsystem* link = connection();
	if (link == nullptr)
	{
		return;
	}

	link->loginServer().onStepChanged.BindWeakLambda(this, [this](Network::ELoginServerStep step)
	{
		UServerConnectionSubsystem* subsystem = connection();
		if (step == Network::ELoginServerStep::ServerSelect)
		{
			// One game server is all this client has met, so it takes the first one up. A list comes when a second exists.
			const Network::FServerListEntry* online = subsystem->loginServer().servers().FindByPredicate([](const Network::FServerListEntry& entry)
			{
				return entry.status == Network::EServerStatus::Online;
			});

			if ((online == nullptr) || !subsystem->loginServer().selectServer(online->serverId))
			{
				_login->setBusy(false);
				_login->setStatus(LOCTEXT("NoServer", "No game server is online."));
			}
		}
		else if (step == Network::ELoginServerStep::TokenIssued)
		{
			subsystem->startGameServer();
		}
	});

	link->loginServer().onFailed.BindWeakLambda(this, [this](const FString& reason)
	{
		if (_login.IsValid())
		{
			_login->setBusy(false);
			_login->setStatus(FText::FromString(reason));
		}
	});

	link->gameServer().onStepChanged.BindWeakLambda(this, [this](Network::EGameServerStep step)
	{
		if (step == Network::EGameServerStep::CharacterSelect)
		{
			showCharacters();

			const Network::FCharacterListEntry* wanted = connection()->gameServer().characters().FindByPredicate([this](const Network::FCharacterListEntry& entry)
			{
				return entry.name == _autoEnter;
			});

			if (wanted != nullptr)
			{
				_autoEnter.Reset();
				connection()->gameServer().selectCharacter(wanted->objectId);
			}
		}
		else if (step == Network::EGameServerStep::Selected)
		{
			travelToWorld(this);
		}
	});

	link->gameServer().onFailed.BindWeakLambda(this, [this](const FString& reason)
	{
		showLogin();
		_login->setStatus(FText::FromString(reason));
	});

	link->gameServer().onCreateRejected.BindWeakLambda(this, [this](Network::ECreateRejectReason reason)
	{
		if (_create.IsValid())
		{
			_create->setStatus(FText::FromString(Network::createRejectReasonName(reason)));
		}
	});

	link->gameServer().onDeleteRejected.BindWeakLambda(this, [this]()
	{
		if (_characters.IsValid())
		{
			_characters->setStatus(LOCTEXT("DeleteRefused", "That character cannot be deleted."));
		}
	});
}

void AFrontEndGameMode::unbindFlows()
{
	UServerConnectionSubsystem* link = connection();
	if (link == nullptr)
	{
		return;
	}

	link->loginServer().onStepChanged.Unbind();
	link->loginServer().onFailed.Unbind();
	link->gameServer().onStepChanged.Unbind();
	link->gameServer().onFailed.Unbind();
	link->gameServer().onCreateRejected.Unbind();
	link->gameServer().onDeleteRejected.Unbind();
}

#undef LOCTEXT_NAMESPACE
