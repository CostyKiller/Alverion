<p align="center">
  <img src="Images/ConceptArt.png" width="100%" alt="Alverion, a fantasy MMORPG">
</p>

Unreal Engine 5 client for L2J Mobius.

## What it does today

- Logs in through the login server: Blowfish and RSA as the stock client does them.
- Menus for login, server select, character select and creation, options and an in-game menu, in front of a small scene, `L_Login`.
- Lists, creates and deletes characters, and enters the world.
- The world is `L_Continent`, a World Partition map with Mesh Terrain. Start Island, its first land, sits under the server's human starting area so its spawns stand on something. Shape it in the editor's Mesh Terrain mode.
- Runs in keyboard mode, where the server takes the client's position as true. Other players, townspeople and monsters walk where the server sends them.
- Characters are coloured placeholders for now. MetaHumans are next, and the server patch already stores the appearance they will wear.

## Layout

| Folder | What |
| --- | --- |
| `Client` | The Unreal Engine 5.8 project. `Network` speaks the server protocol, `Alverion` is the game. |
| `ServerPatch` | A patch for `L2J_Mobius` that stores each character's appearance block. See its README. |

## Running it

1. Run an L2J Mobius login server and game server, with the patch applied and `character_appearance.sql` imported.
2. Build `Client/Alverion.uproject` with Unreal Engine 5.8 and run it.
3. Log in. A server with `AutoCreateAccounts` on makes the account the first time.

Development builds also accept `-autologin=account,password,character`.

Run it from the editor executable with `-game`, or package it:

```
RunUAT BuildCookRun -project=Client/Alverion.uproject -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=Client/Saved/Packaged
```

Play in Editor shows no ground on `L_Continent`, because the editor previews the terrain instead of streaming its compiled sections. After changing the terrain, compile those sections again:

```
UnrealEditor-Cmd Client/Alverion.uproject /Game/Maps/L_Continent -run=WorldPartitionBuilderCommandlet -Builder=WorldPartitionMeshPartitionBuilder -SCCProvider=None
```

## Tests

```
UnrealEditor-Cmd Client/Alverion.uproject -ExecCmds="Automation RunTests Alverion;Quit" -nullrhi
```

`Alverion.Network` needs nothing. `Alverion.Live` needs both servers running on this machine.

## Licence

The client is MIT licensed, see `LICENSE`.
