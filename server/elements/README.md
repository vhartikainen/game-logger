# elements/

Per-player assets referenced by the view pages
([currentlyplaying.php](../currentlyplaying.php), [gamelogger.php](../gamelogger.php))
and the status monitor ([monitor.html](../monitor.html)):

- `<player>-avatar.jpg` — avatar shown next to the player
- `<player>.mp3` — alarm sound played by monitor.html when that player's data goes stale
- `fail.mp3` — shared "failure" sting played alongside the per-player alarm

The originals were never committed to the repo. The files here are **generated
placeholders** for local development:

- Avatars: 200×200 solid-color JPGs with the player's white initial.
- Sounds: short single sine tones, a distinct frequency + length per player
  (`fail.mp3` is a two-tone descending buzzer).

Regenerated with ffmpeg — see the commands in the project history / Docker docs.
Drop in real assets with the same filenames to replace them.

Current players: mikko, juha, sami, pavel, tero, ville.
