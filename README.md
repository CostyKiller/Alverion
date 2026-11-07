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
- Characters are the MetaHumans their stored appearance describes, in character select and creation and in the world. They idle, walk, run and fall.

## Layout

| Folder | What |
| --- | --- |
| `Client` | The Unreal Engine 5.8 project. `Network` speaks the server protocol, `Alverion` is the game. |
| `ServerPatch` | A patch for `L2J_Mobius` that stores each character's appearance block. See its README. |

## MetaHumans

The characters are MetaHumans, about 3 GB, so they are not in the repository. Download them here:

https://drive.google.com/file/d/1DZWQk5yhEA7fCeuYtKLhv3QjR7KzBW7A/view?usp=sharing

Extract the zip at the root of the repository, so that its folders land in `Client/Content/Characters/MetaHumans/` (`Grooms` and `Unpacked`). Git ignores that folder. Without it the game still runs, with coloured placeholder figures standing in for the characters.

MetaHumans are covered by the Unreal Engine EULA: share the download only with people who have accepted it.

## Animations

The character animations are Quaternius's Universal Animation Library, packs 1 and 2, baked onto the MetaHuman skeleton. Download them here:

https://drive.google.com/file/d/1_vLGT5tUYuHbd2LIAYOOLMSSs5ld1u2A/view?usp=sharing

Extract the zip so that the clips land in `Client/Content/Characters/Animations/UAL/`. The packs are CC0, so no credit is required.

## Running it

1. Run an L2J Mobius login server and game server, with the patch applied and `character_appearance.sql` imported.
2. Download the MetaHumans and the animations, see above.
3. Build `Client/Alverion.uproject` with Unreal Engine 5.8 and run it.
4. Log in. A server with `AutoCreateAccounts` on makes the account the first time.

Development builds also accept `-autologin=account,password,character`.

The menus run straight from the editor, but the world runs only in a packaged build: its terrain plugin needs an editor library to read the map, and that library cannot run inside a game.

```
RunUAT BuildCookRun -project=Client/Alverion.uproject -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory=Client/Saved/Packaged
```

After changing the terrain, compile its sections again before packaging:

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
