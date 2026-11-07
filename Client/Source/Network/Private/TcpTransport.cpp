#include "Transport.h"

#include "IPAddress.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

namespace Network
{
namespace
{
	// Short enough that a stop request is noticed at once, long enough that an idle link does not spin.
	constexpr double receiveWaitMilliseconds = 5.0;

	constexpr int32 receiveChunkBytes = 16384;
}

FTcpTransport::~FTcpTransport()
{
	close();
}

bool FTcpTransport::connect(const FString& host, int32 port)
{
	ISocketSubsystem* sockets = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (sockets == nullptr)
	{
		return false;
	}

	TSharedPtr<FInternetAddr> address = sockets->GetAddressFromString(host);
	if (!address.IsValid() || !address->IsValid())
	{
		FAddressInfoResult resolved = sockets->GetAddressInfo(*host, nullptr, EAddressInfoFlags::Default, NAME_None);
		if ((resolved.ReturnCode != SE_NO_ERROR) || resolved.Results.IsEmpty())
		{
			return false;
		}

		address = resolved.Results[0].Address;
	}

	address->SetPort(port);

	_socket = sockets->CreateSocket(NAME_Stream, TEXT("Alverion"), address->GetProtocolType());
	if (_socket == nullptr)
	{
		return false;
	}

	_socket->SetNoDelay(true);

	if (!_socket->Connect(*address))
	{
		close();
		return false;
	}

	return true;
}

EReceiveResult FTcpTransport::receive(TArray<uint8>& into)
{
	if (_socket == nullptr)
	{
		return EReceiveResult::Failed;
	}

	if (!_socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromMilliseconds(receiveWaitMilliseconds)))
	{
		return EReceiveResult::Healthy;
	}

	uint8 chunk[receiveChunkBytes];
	int32 read = 0;
	if (!_socket->Recv(chunk, receiveChunkBytes, read))
	{
		return EReceiveResult::Failed;
	}

	// Readable with nothing to read is how a socket reports the other end hanging up.
	if (read == 0)
	{
		return EReceiveResult::ClosedByPeer;
	}

	into.Append(chunk, read);
	return EReceiveResult::Healthy;
}

bool FTcpTransport::send(TArrayView<const uint8> bytes)
{
	int32 sentTotal = 0;
	while (sentTotal < bytes.Num())
	{
		int32 sent = 0;
		if ((_socket == nullptr) || !_socket->Send(bytes.GetData() + sentTotal, bytes.Num() - sentTotal, sent))
		{
			return false;
		}

		sentTotal += sent;
	}

	return true;
}

void FTcpTransport::close()
{
	if (_socket == nullptr)
	{
		return;
	}

	_socket->Close();
	ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(_socket);
	_socket = nullptr;
}
}
