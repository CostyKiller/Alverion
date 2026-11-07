#include "LoginServerFlow.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlverionLoginServer, Display, All);

namespace Network
{
FLoginServerFlow::~FLoginServerFlow()
{
	end();
}

void FLoginServerFlow::begin(FSession& session, const FString& account, const FString& password)
{
	_session = &session;
	_password = password;
	_modulus.Reset();
	_selectedServer = 0;
	_servers.Reset();
	_token = FLoginToken();
	_token.account = account;
	_lastError.Reset();

	_session->setHandler(FPacketHandler::CreateRaw(this, &FLoginServerFlow::handlePacket));
	setStep(ELoginServerStep::Connecting);
}

void FLoginServerFlow::end()
{
	if (_session != nullptr)
	{
		_session->setHandler(FPacketHandler());
		_session = nullptr;
	}

	scrubPassword();
	_step = ELoginServerStep::Idle;
}

void FLoginServerFlow::tick()
{
	if ((_session == nullptr) || (_step == ELoginServerStep::Idle) || (_step == ELoginServerStep::Failed) || (_step == ELoginServerStep::TokenIssued))
	{
		return;
	}

	if (_session->hasFailed())
	{
		fail(_session->lastError());
	}
	else if (_session->state() == ESessionState::Disconnected)
	{
		// The login server hangs up on refusals it does not explain, so silence before the token is a failure too.
		fail(TEXT("The login server closed the connection."));
	}
}

bool FLoginServerFlow::selectServer(uint8 serverId)
{
	if ((_session == nullptr) || (_step != ELoginServerStep::ServerSelect))
	{
		return false;
	}

	_selectedServer = serverId;

	FWriteBuffer buffer;
	LoginServer::writeRequestServerLogin(buffer, _token.loginOk1, _token.loginOk2, serverId);
	send(buffer);

	setStep(ELoginServerStep::SelectSent);
	return true;
}

void FLoginServerFlow::handlePacket(uint32 opcode, TArrayView<const uint8> body)
{
	FReadBuffer buffer(body);

	switch (opcode)
	{
		case LoginServer::Opcode::init:
		{
			LoginServer::FInit init;
			if ((_step != ELoginServerStep::Connecting) || !init.read(buffer))
			{
				fail(TEXT("Init was malformed or out of turn."));
				return;
			}

			static_cast<FLoginCipher*>(_session->cipher())->setSessionKey(init.blowfishKey);

			_modulus = init.modulus;
			ServerCrypt::unscrambleModulus(_modulus);

			FWriteBuffer reply;
			LoginServer::writeAuthGameGuard(reply, init.sessionId);
			send(reply);

			setStep(ELoginServerStep::Authenticating);
			return;
		}
		case LoginServer::Opcode::ggAuth:
		{
			FWriteBuffer reply;
			const bool written = LoginServer::writeRequestAuthLogin(reply, _modulus, _token.account, _password);
			scrubPassword();

			if (!written)
			{
				fail(TEXT("The credentials could not be encrypted."));
				return;
			}

			send(reply);
			return;
		}
		case LoginServer::Opcode::loginOk:
		{
			buffer.readInt32(_token.loginOk1);
			buffer.readInt32(_token.loginOk2);
			if (buffer.hasFailed())
			{
				fail(TEXT("LoginOk was malformed."));
				return;
			}

			FWriteBuffer reply;
			LoginServer::writeRequestServerList(reply, _token.loginOk1, _token.loginOk2);
			send(reply);

			setStep(ELoginServerStep::ServerListRequested);
			return;
		}
		case LoginServer::Opcode::serverList:
		{
			LoginServer::FServerList list;
			if (!list.read(buffer))
			{
				fail(TEXT("The server list was malformed."));
				return;
			}

			_servers = MoveTemp(list.servers);
			setStep(ELoginServerStep::ServerSelect);
			return;
		}
		case LoginServer::Opcode::playOk:
		{
			const FServerListEntry* server = _servers.FindByPredicate([this](const FServerListEntry& candidate)
			{
				return candidate.serverId == _selectedServer;
			});

			buffer.readInt32(_token.playOk1);
			buffer.readInt32(_token.playOk2);
			if ((_step != ELoginServerStep::SelectSent) || buffer.hasFailed() || (server == nullptr))
			{
				fail(TEXT("PlayOk was malformed or out of turn."));
				return;
			}

			_token.host = server->host;
			_token.port = server->port;
			setStep(ELoginServerStep::TokenIssued);
			return;
		}
		case LoginServer::Opcode::loginFail:
		case LoginServer::Opcode::accountKicked:
		{
			int32 code = 0;
			buffer.readInt32(code);
			fail(FString::Printf(TEXT("The login server refused: %s."), *loginFailReasonName(code)));
			return;
		}
		case LoginServer::Opcode::playFail:
		{
			uint8 code = 0;
			buffer.readUInt8(code);
			fail(FString::Printf(TEXT("The game server cannot be entered: %s."), *loginFailReasonName(code)));
			return;
		}
		default:
		{
			return;
		}
	}
}

void FLoginServerFlow::send(const FWriteBuffer& buffer)
{
	if (_session != nullptr)
	{
		_session->send(buffer.bytes());
	}
}

void FLoginServerFlow::setStep(ELoginServerStep next)
{
	if (_step == next)
	{
		return;
	}

	_step = next;
	UE_LOG(LogAlverionLoginServer, Display, TEXT("Login server step %d."), static_cast<int32>(next));
	onStepChanged.ExecuteIfBound(next);
}

void FLoginServerFlow::fail(const FString& reason)
{
	if (_lastError.IsEmpty())
	{
		_lastError = reason;
	}

	UE_LOG(LogAlverionLoginServer, Display, TEXT("%s"), *_lastError);

	setStep(ELoginServerStep::Failed);
	scrubPassword();

	if (_session != nullptr)
	{
		_session->disconnect();
	}

	onFailed.ExecuteIfBound(_lastError);
}

void FLoginServerFlow::scrubPassword()
{
	if (!_password.IsEmpty())
	{
		FMemory::Memzero(_password.GetCharArray().GetData(), _password.GetCharArray().Num() * sizeof(TCHAR));
		_password.Empty();
	}
}

ELoginServerStep FLoginServerFlow::step() const
{
	return _step;
}

const TArray<FServerListEntry>& FLoginServerFlow::servers() const
{
	return _servers;
}

const FLoginToken& FLoginServerFlow::token() const
{
	return _token;
}

const FString& FLoginServerFlow::lastError() const
{
	return _lastError;
}
}
