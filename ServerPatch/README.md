# Alverion appearance extension for L2J Mobius

The Alverion client builds MetaHuman characters, and the server keeps only a face, a hair style
and a hair colour per character. This patch lets the server store the client's own appearance
block, 24 bytes today, and hand it back, without changing anything a client that sends no block
sees.

## What it changes

- `CharacterCreate` reads an optional block after the stock fields: the int `0x31564C41` (the bytes
  `ALV1`), a length byte, then the block. A client that sends nothing after the face is unaffected.
- The block is stored in its own table, `character_appearance`, keyed by character id, and cached.
- `CharSelectionInfo` and `CharInfo` append a trailer at the very end of the packet: every block,
  padded to the longest, then that length and the count as shorts, then the magic. Nothing is
  appended when no listed character has a block, so stock characters produce stock packets.
- Deleting a character deletes its block.

Four upstream files change by 30 lines in total, and one class and one SQL file are added:

| File | Change |
| --- | --- |
| `java/.../network/clientpackets/CharacterCreate.java` | read the block, store it after creation |
| `java/.../network/serverpackets/CharSelectionInfo.java` | trailer with one block per character |
| `java/.../network/serverpackets/CharInfo.java` | trailer with the player's block |
| `java/.../network/GameClient.java` | delete the block with the character |
| `java/.../data/sql/CharacterAppearanceTable.java` | new: storage, cache, trailer writer |
| `dist/db_installer/sql/game/character_appearance.sql` | new: the table |

## Applying it

From the root of the L2J Mobius server project:

```
git apply --check C:/Alverion/ServerPatch/alverion-appearance.patch
git apply C:/Alverion/ServerPatch/alverion-appearance.patch
```

Then rebuild the server and run `dist/db_installer/sql/game/character_appearance.sql` against the
game database (the database installer picks it up with the other game tables).

## Running it without applying it

The patched classes can be compiled on their own and put ahead of the stock ones, which is how it
is tested here without touching the workspace:

```
javac -encoding UTF-8 -nowarn -proc:none -cp "<Server>/bin;<Server>/dist/libs/*" -d build/classes <the five .java files>
java -cp "build/classes;<Server>/bin;<Server>/dist/libs/*" org.l2jmobius.gameserver.GameServer
```

The second command runs from `<Server>/dist/game`, with the JVM options in its `java.cfg`.

## Layout of this repository

The first commit holds the four upstream files untouched; the second holds this extension. The
patch file is the diff between them, so it can be regenerated with
`git diff <first commit> HEAD -- java dist > alverion-appearance.patch`.
