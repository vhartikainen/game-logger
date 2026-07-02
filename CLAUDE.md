# game-logger

## server/gamelogger/settings.txt

Config file served as-is to the client (`clientConnect.php` just `readfile()`s it). Client parses it in `client/GameLogger/settings.cpp` — a line starting with `!` is a comment (whole line ignored, regardless of what follows), `#` is a general setting, a leading number is a game entry. Format is documented in the file's own header comment.

Game entry format (tab-separated):
```
<Game ID>	<Executable filename>	<Long name>	<Icon URL>
```

Reserved/existing IDs not to reuse: `99` (notepad), `100` (commented-out legacy calc.exe), `101` (CalculatorApp.exe). Real game entries currently run `1`-`65`, `66`-`98`, `102`-`113`. When adding new games, continue from the highest unused ID, skipping the reserved block.

### Syncing with locally installed Steam games

When asked to add/sync installed Steam games into settings.txt:

- Steam library on this machine: `C:\Program Files (x86)\Steam\steamapps`
- Each `appmanifest_<appid>.acf` (ignore `steamapps\workshop\*.acf`) has `appid`, `name`, `installdir` fields (VDF/KeyValue format).
- Game files live under `steamapps\common\<installdir>\`. The primary executable isn't always at the install root — check for a shipping binary first (e.g. `Binaries\Win64\<Name>-Win64-Shipping.exe` for Unreal Engine games), and skip launchers, anti-cheat exes (`*_BE.exe`, `EasyAntiCheat*`), crash handlers, redistributable installers, and editor/CLI tools.
- Icon URL convention used so far: `https://cdn.akamai.steamstatic.com/steam/apps/<appid>/header.jpg`
- Skip apps with no real executable (e.g. appid `228980`, "Steamworks Common Redistributables").
- Golf With Your Friends (appid `431240`) and Predecessor (appid `961200`) are already in the file as IDs `63`/`64` — don't re-add.
- A handful of entries run nested Shipping binaries rather than root launchers (PUBG, CS2, Enlisted, War Thunder, Deep Rock Galactic, Tribes of Midgard, LORT, StarRupture, The Headliners) — worth confirming the actual process name against Task Manager if the logger doesn't detect them.
