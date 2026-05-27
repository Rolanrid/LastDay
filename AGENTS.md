# AGENTS.md — LastDay (UE 5.7.4)

## Project structure

Single UE5 module `LastDay` with four logical areas sharing no explicit module boundary:

| Area | Directory | Key classes |
|---|---|---|
| Core | `Source/LastDay/` | `ALastDayGameMode`, `ALastDayCharacter`, `ALastDayPlayerController`, `ALastDayCameraManager` |
| Gameplay | `Source/LastDay/Public/` | `AUnit`, `ATurret`, `UTurretSocketComponent`, `ABaseProjectile` |
| Horror variant | `Source/LastDay/Variant_Horror/` | Stamina sprinting, flashlight |
| Shooter variant | `Source/LastDay/Variant_Shooter/` | Weapons, AI (StateTree), NPC spawner, pickups, team scores |

`PublicIncludePaths` in `LastDay.Build.cs` explicitly adds every sub-folder. New files in these directories are found without relative-path includes.

## Key conventions & quirks

- **AI uses StateTree** (plugins `StateTree` + `GameplayStateTree`), **not** Behavior Trees. Custom tasks/conditions live in `ShooterStateTreeUtility.h/.cpp`.
- **Enhanced Input** — all player controllers manage `UEnhancedInputLocalPlayerSubsystem` and `UInputMappingContext`s.
- **No unit tests, no CI** (`.github/workflows/` is empty).
- **No Plugins/ directory** — all enabled plugins are Engine plugins.
- **Log category**: `LogLastDay` (declared in `LastDay.h`, defined in `LastDay.cpp`).
- **Camera pitch** clamped `-70` to `+80` in `ALastDayCameraManager`.
- **Collision profile** uses a custom `Projectile` channel (set in `DefaultEngine.ini`).

## Important files outside source

| File | Purpose |
|---|---|
| `Doc/` | Game design docs, task lists, dev log, meeting notes (Chinese) |
| `Doc/策划案.txt` | Full game design doc: base-building survival FPS |
| `Doc/备忘录.txt` | Dev notes, design brainstorming |
| `Doc/Demo待做任务列表.md` | Demo todo list with Sprint plan (94 sub-tasks) |
| `Doc/背景设定.md` | Game world background / lore |
| `Doc/开发日志.md` | Development log |
| `Doc/启动会议.md` | Kick-off meeting notes |
| `LastDay.uproject` | Engine 5.7.4, plugin list, module ref |
| `.vsconfig` | Required VS workload for UE (auto-install with VS) |

## Build & run

Standard UE5 workflow:
```
Generate project files → open LastDay.uproject or LastDay.sln → build
```

Startup map: `/Game/FirstPerson/Lvl_FirstPerson`

Default game mode BP: `/Game/FirstPerson/Blueprints/BP_FirstPersonGameMode`

No custom build scripts, no codegen steps, no package manager.

## Git

Origin: `https://github.com/Rolanrid/LastDay`
`.gitignore` covers all UE standard artifacts. `Config/DefaultInput.ini` is **tracked** (contains Enhanced Input bindings).
