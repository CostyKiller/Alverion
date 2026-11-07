#pragma once

/** Every login server packet this client reads or writes, one per file under ClientPackets and ServerPackets. */
#include "LoginServer/LoginServerProtocol.h"
#include "LoginServer/ServerPackets/Init.h"
#include "LoginServer/ServerPackets/ServerList.h"
#include "LoginServer/ClientPackets/AuthGameGuard.h"
#include "LoginServer/ClientPackets/RequestAuthLogin.h"
#include "LoginServer/ClientPackets/RequestServerList.h"
#include "LoginServer/ClientPackets/RequestServerLogin.h"
