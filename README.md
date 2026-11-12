# GameFixes_mm

> [!IMPORTANT]
> **Forked**. Original: [SlynxCZ/GameFixes_mm](https://github.com/SlynxCZ/GameFixes_mm).
> Also check out their [Source2Mod](https://www.source2mod.net) version: [GameFixes_s2m](https://github.com/SlynxCZ/GameFixes_s2m).

Metamod:Source plugin with CS2 server fixes.

Each is a block in `addons/game_fixes/game_fixes.ini`, on with `"enable" "1"`, a missing block is off.

A fix that can't find what it needs is logged and left off.

## Fixes

| Block | Fixes |
| --- | --- |
| `abstract_entity` | Creating an entity whose class can't be constructed (`weapon_cs_base` in old map ports) crashes, such classnames are refused like unknown ones. |
| `beam_crash` | `CBeam::SetBeamOrigin` / `SetBeamEndPos` loop forever on a beam without a parent. |
| `demo_record` | With `tv_enable 1`, GOTV gets its own slot and is never disconnected, so recording survives match restarts. |
| `hammer_id` | `SetSchemaHammerUniqueId` skips `m_sUniqueHammerID` on most entities. |
| `slow_animation` | `curtime` loses precision after a day or two on one map, the map is reloaded while the server is empty, keeping `mp_timelimit`. |
| `steam_ban` | The engine's GC ban / cooldown kick spares `whitelist` accounts and those other plugins allow (`IGameFixes`), holding a new one for `hold_seconds`; `clear_after_pass` drops stale entries. |
| `team_limit` | Joins are never refused as "team full", also on maps with few spawns. Autobalance is untouched. |
| `input_activator_crash` | A `TestActivator` input without an activator crashes the server. |
| `sv_cheats` | Turning `sv_cheats` off leaves noclip on. |
| `rampbug` | Recudes surf rampbugs (CS2KZ's fix, via CS2Fixes-RampbugFix). |
| `water_jump` | A jump out of water fails after touching its bottom. |
| `voice_flood` | Malformed or oversized voice messages, each client's voice rate is capped. Nobody is kicked. |
| `server_list_players` | The server browser lists no players ([Source2ZE/ServerListPlayersFix](https://github.com/Source2ZE/ServerListPlayersFix)). |

## Interface for other plugins

```cpp
IGameFixes* pGameFixes = static_cast<IGameFixes*>(g_SMAPI->MetaFactory(GAMEFIXES_INTERFACE_VERSION, nullptr, nullptr));
pGameFixes->SteamBanAllowAccount(steamId64);
pGameFixes->SteamBanDisallowAccount(steamId64);
```

## Building

```bash
git submodule update --init --recursive
mkdir build && cd build
python ../configure.py --enable-optimize
ambuild
```

The package lands in `build/package/cs2/`.

## Author

Slynx (˙·٠● S l y n x ●٠·˙)
[https://slynxdev.cz](https://slynxdev.cz)
