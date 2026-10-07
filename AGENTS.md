# AGENTS.md — LastDay (UE 5.7.4)

## Project structure

Single UE5 module `LastDay` with four logical areas sharing no explicit module boundary:

| Area | Directory | Key classes |
|---|---|---|
| Core | `Source/LastDay/` | `ALastDayGameMode`, `ALastDayCharacter`, `ALastDayPlayerController`, `ALastDayCameraManager` |
| Unit (base) | `Source/LastDay/Unit/` | `AUnit` |
| Construction | `Source/LastDay/Construction/` | `ATurret`（索敌/转向/开火指挥；炮口是它 BeginPlay 时生成并挂上去的 Actor） |
| Explosions | `Source/LastDay/Explosions/` | `AExplosion`（标记基类）, `AExplosionEffect`（膨胀+淡出的球体表现，`Damage > 0` 时结算范围伤害） |
| Weapons | `Source/LastDay/Weapons/` | `AWeapon`（标记基类）, `ASimpleWeapon`（炮管+伤害/间隔/子弹的通用实现）, `ACannon`（炮塔武器）, `AMonsterWeapon`（怪物武器）, `UMiningDamageType`, `AShooterWeapon`, `AShooterPickup`, `AShooterWeaponHolder` |
| Projectiles | `Source/LastDay/Projectiles/` | `AProjectile`（标记基类）, `ACannonBall`（炮塔炮弹，原 `ABaseProjectile`）, `AShooterProjectile` |
| Monster / Demo | `Source/LastDay/Demo/` | `AMonster`（红色三角怪，走导航网格找最近的敌人——敌方 AUnit 或玩家——并开火）, `AEnemySpawner`（以玩家为中心刷怪）, `ADemoEnemy`（旧球体敌人，保留未用）, `ASourceCrystal`, `ACollectorBuilding`, `ALastDayGameState`, `ADayNightCycleManager` |
| Horror variant | `Source/LastDay/Variant_Horror/` | Stamina sprinting, flashlight |
| Shooter variant | `Source/LastDay/Variant_Shooter/` | AI (StateTree), NPC spawner, team scores |

`PublicIncludePaths` in `LastDay.Build.cs` explicitly adds every sub-folder. New files in these directories are found without relative-path includes.

## Key conventions & quirks

- **AI uses StateTree** (plugins `StateTree` + `GameplayStateTree`), **not** Behavior Trees. Custom tasks/conditions live in `ShooterStateTreeUtility.h/.cpp`.
- **Enhanced Input** — all player controllers manage `UEnhancedInputLocalPlayerSubsystem` and `UInputMappingContext`s.
- **No unit tests, no CI** (`.github/workflows/` is empty).
- **No Plugins/ directory** — all enabled plugins are Engine plugins.
- **Log category**: `LogLastDay` (declared in `LastDay.h`, defined in `LastDay.cpp`).
- **Camera pitch** clamped `-70` to `+80` in `ALastDayCameraManager`.
- **Collision profile** uses a custom `Projectile` channel (set in `DefaultEngine.ini`); the `Projectile` profile has `Projectile → Projectile = Ignore`, so bullets never block each other (no runtime bullet registry needed).
- **Damage flow**: attackers use `UGameplayStatics::ApplyDamage` / `ApplyPointDamage` / `ApplyRadialDamage`; receivers only override `AActor::TakeDamage` (the old `IHittable`/`Hitted` interface is gone).
- **Turret architecture**: `ATurret` does detection (XY-plane angle/radius, separate Z height range), aiming and fire command; `ACannon` only holds damage / fire interval and spawns the projectile.
- **Marker base classes**: `AWeapon` / `AProjectile` / `AExplosion` are empty bases kept only for `IsA<>` checks (projectiles ignore each other via `IsA<AProjectile>()`). `ACannon` is an `AWeapon` actor spawned by the turret at `BeginPlay` and attached to it.
- **Teams**: `EUnitTeam` (Player = 0, Monster = 1) lives in `Unit.h`. Turrets and monsters only target `AUnit`s of a *different* team, so player buildings are never shot by the player's turrets and monsters are valid turret targets.
- **Player tools**: number keys 1-4 / mouse wheel switch `EPlayerTool` (Gun / MiningTool / BuildTurret / BuildCollector) on `ALastDayCharacter`; only the mining tool uses `UMiningDamageType`, which is the only damage type crystals gather from (units ignore mining damage), so guns cannot harvest crystals.
- **Monster AI**: `AMonster` follows a navmesh path (`UNavigationSystemV1::FindPathSync`); without nav data it falls back to six-way swept steering. Movement is always swept (`AddActorWorldOffset(..., bSweep=true)`), so monsters can never pass through walls. Targets are the nearest enemy — any `AUnit` of another team, or the living player character. A locked target is kept for `TargetSwitchCooldown` seconds and is only replaced when the new candidate is closer by `SwitchDistanceAdvantage`, which stops the monster from flicking between two similar targets.
- **Day/night**: `ADayNightCycleManager` drives the sky from one continuous time axis (no per-phase switching). The sun light is atmosphere-sun-light 0 (no sun disc at night, it sits below the horizon); a separate moon light is atmosphere-sun-light 1 so it lights the night sky and clouds (toggle `bMoonLightsSky`), casts no shadows, and lights the scene after dark. All lights are forced to `Movable`; night uses a dim deep blue-violet ambient floor.
- **White-box colors**: the palette lives in `Source/LastDay/LastDayVisuals.h` (construction & weapons = yellow, source crystal = blue, enemy = purple, projectile = red; the explosion keeps its orange-red).

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
