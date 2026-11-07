<p align="center">
  <img src="Images/ConceptArt.png" width="100%" alt="Alverion, a fantasy MMORPG">
</p>

Unreal Engine 5 client for L2J Mobius.

## What it does today

- Logs in through the login server: Blowfish and RSA as the stock client does them.
- Lists, creates and deletes characters, and enters the world.
- Start Island: a crude island under the server's human starting area, its coastlines and hills traced by hand, so its spawns stand on something.
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

## Tests

```
UnrealEditor-Cmd Client/Alverion.uproject -ExecCmds="Automation RunTests Alverion;Quit" -nullrhi
```

`Alverion.Network` needs nothing. `Alverion.Live` needs both servers running on this machine.

## Licence

The client is MIT licensed, see `LICENSE`.
