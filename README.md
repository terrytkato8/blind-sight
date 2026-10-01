[README.md](https://github.com/user-attachments/files/32933438/README.md)
# Blind Sight — Unreal Engine C++ Scaffold

**Kato.8 Studios · UE 5.4 · GDD v0.1 (Sept 16, 2026)**

This is the drop-in C++ source for the Blind Sight vertical slice: the server-authoritative sound-event
system, GAS-backed Hunter/Hider roles, both game modes (Blackout + Manhunt) with scoring, in-map round
rotation, throwables/resupply, spectate policy, HUD bases, and session plumbing. Everything that must be a
UAsset (meshes, MetaSounds, Input Actions, widgets, maps) is left for the editor — the checklist below tells
you exactly what to create and where to plug it in.

---

## 1. Getting it building

1. Copy this folder to your projects directory (or drop `Source/`, `Config/`, `BlindSight.uproject` into a
   fresh Blank C++ project named **BlindSight**).
2. Right-click `BlindSight.uproject` → **Generate Visual Studio project files**.
3. Open the `.sln`, set config **Development Editor**, build. Requires the plugins already enabled in the
   `.uproject`: GameplayAbilities, EnhancedInput, Metasound, CommonUI, OnlineSubsystemEOS.
4. Launch the editor. Expect one warning until you create `DA_SoundProfile` (step 2.1).

`Config/DefaultEngine.ini` points at maps that don't exist yet (`L_MainMenu`, `L_Lobby`, `L_Slice01`).
Either create them or change the paths.

---

## 2. Editor checklist (assets you need to make)

### 2.1 Sound (GDD §6.1, §10)
| Asset | Path | Notes |
|---|---|---|
| `DA_SoundProfile` | `/Game/Data/` | Data Asset of class **BSSoundProfile**. Ships with GDD baseline radius/intensity. Assign a MetaSound to each event's `Sound`. It's referenced in `DefaultGame.ini` and Project Settings › Game › Blind Sight. |
| `MS_Footstep`, `MS_Throw*`, `MS_Door`, `MS_Gunshot`, `MS_Pulse` | `/Game/Audio/` | MetaSound Sources. Each should expose float inputs **`Intensity`** and **`Radius`** — the GameState sets them at play time so loudness is driven by gameplay, not baked. Use an Attenuation asset with **occlusion enabled** on each. |

### 2.2 Characters
| Asset | Parent | Assign |
|---|---|---|
| `BP_HunterCharacter` | `BSHunterCharacter` | Mesh, `IMC_Default` + all `IA_*`, **Startup Abilities**: `BSGA_Fire`, `BSGA_Pulse`. Optionally add `PP_HunterVision` to the VisionPostProcess component's Blendables for the true short-sight-radius blur (see §4). |
| `BP_HiderCharacter` | `BSHiderCharacter` | Mesh (must have **Render CustomDepth** allowed for Pulse silhouettes), same inputs, **Startup Abilities**: `BSGA_Throw` (or a BP child of it with your `BP_Throwable_*` class). Implement `BP_OnEliminated` for the "downed" comedy beat. |
| `BP_PlayerController` | `BSPlayerController` | `HunterHUDClass`, `HiderHUDClass`, `ResultsWidgetClass`. Turn off `bDebugDrawPings` once the HUD renders pings. |

### 2.3 Input (Enhanced Input) — `/Game/Input/`
`IMC_Default` with: `IA_Move` (Axis2D, WASD), `IA_Look` (Axis2D, mouse), `IA_Primary` (LMB — Fire / Throw),
`IA_Secondary` (RMB or E — Pulse / Interact+Pickup), `IA_Sprint` (Shift, hold), `IA_Crouch` (Ctrl/C, toggle).

### 2.4 Game modes
| Asset | Parent | Set |
|---|---|---|
| `BP_GM_Blackout` | `BSGameMode_Blackout` | `HunterPawnClass`/`HiderPawnClass` → your BPs. Tune `RoundSeconds`, `BaseAmmo`, `Scoring`. |
| `BP_GM_Manhunt` | `BSGameMode_Manhunt` | Same pawn classes. Tune `ExtraAmmo`, `Scoring`, optional `HardCapSeconds`. |
| `BP_GM_Lobby` | `BSLobbyGameMode` | Call `StartMatch(MapName, ModeClass)` from your lobby UI. |
Set `GlobalDefaultGameMode` / per-map override to the BP versions.

### 2.5 World actors (drop in `L_Slice01`)
- **PlayerStart** ×N with `PlayerStartTag = Hunter` (one central spot) and `Hider` (scattered). Untagged starts are fallbacks.
- `BP_ThrowableSpawn` (parent `BSThrowableSpawnPoint`): mix safe + exposed placements (GDD §8). `InitialStock` 2–4.
- `BP_Throwable_Bottle/Can/Rock` (parent `BSThrowable`): mesh + `ImpactIntensityScale` per prop for distinct signatures.
- `BP_Door` (parent `BSDoor`).
- Make level geometry **block the Visibility channel** — the occlusion trace uses it.

### 2.6 UI — `/Game/UI/`
- `WBP_HunterHUD` (parent `BSHunterHUDWidget`): implement `OnAmmoChanged`, `OnSoundPing` (world-space ripple via
  `ProjectWorldLocationToScreen` or a compass using `GetBearingToLocation`), `OnPulseCooldownUpdated`.
- `WBP_HiderHUD` (parent `BSHiderHUDWidget`): `OnThrowablesChanged`, `OnNoiseTierChanged`, `OnRoundTimeUpdated`, `OnRemainingHidersChanged`, `OnEliminated`.
- `WBP_Results`: read `GameState.GetLastRoundResults()` (`FBSRoundScoreEntry` array) and `GetNextHunterPlayerState()` for the "you're up next" callout. Bind to `OnRoundResults`.

---

## 3. Architecture map

```
Source/BlindSight/
├─ Core/          BSTypes (enums/structs), BSGameplayTags (native tags), BSGameSettings (Project Settings)
├─ Sound/         BSSoundProfile (tunable catalogue) · BSSoundEventSubsystem (server bus, falloff+occlusion,
│                 Hunter-only Client RPC) · BSSoundEmitterComponent (movement → footsteps)
├─ Abilities/     BSAttributeSet (Ammo, Throwables) · BSAbilitySystemComponent · BSGameplayAbility (ServerOnly)
│                 BSGA_Fire · BSGA_Throw · BSGA_Pulse
├─ Characters/    BSCharacterBase (ASC on PlayerState, Enhanced Input) · BSHiderCharacter · BSHunterCharacter
├─ Gameplay/      BSThrowable (Chaos, impact ping) · BSThrowableSpawnPoint (stock) · BSDoor · BSInteractable
├─ GameFramework/ BSGameMode (round state machine, rotation, spawns) · BSGameMode_Blackout · BSGameMode_Manhunt
│                 BSGameState (replicated round state, audio multicast) · BSPlayerState (roles, score, ASC)
│                 BSPlayerController (pings, HUD swap, spectate) · BSScoringLibrary · BSLobbyGameMode
├─ UI/            BSHUDWidgets (Hunter/Hider HUD bases)
└─ Online/        BSGameInstance (Host/Find/Join over IOnlineSession — NULL now, EOS via ini)
```

**Round flow:** `Lobby → StartRound() → HeadStart (Hunter frozen) → InProgress → EndRound(reason) → RoundOver
(results, rotation cursor++) → StartRound()`. Rounds run in-map with respawns, no travel. Hunter rotation is
sequential by join order (GDD §4.3 MVP recommendation). Console: `BSStartRound` on a listen server.

**Sound pipeline:** any server code calls `UBSSoundEventSubsystem::EmitSound(ctx, Type, Origin, Instigator)`.
The subsystem looks up radius/intensity, plays audio on every client through `GameState.MulticastPlaySoundEvent`,
and for each Hunter computes `intensity × falloff^exp × occlusion` and sends `ClientReceiveSoundPing` **only if
above the noise floor**. Hiders never receive ping data → nothing to exploit client-side.

**Anti-cheat posture:** all abilities are `ServerOnly`; ammo/throwables are server-modified attributes;
pings are server-computed; elimination is a server call from the Fire ability's own trace.

---

## 4. Hunter vision (GDD §6.2)
Out of the box `ABSHunterCharacter` applies crushed saturation, heavy vignette, pinned exposure and a shallow
DOF via `UPostProcessComponent` on the local Hunter only. For the intended "near-melee sight radius" look,
create `PP_HunterVision` (Post Process material): sample SceneDepth, fade to near-black/blur beyond
`SightRadius` (expose it as a scalar parameter or push via a Material Parameter Collection), and add it to the
component's Blendables. Pulse silhouettes use Custom Depth stencil 1 — add a stencil-masked outline pass in
the same material.

---

## 5. Tunables (where the balance lives)
| Knob | Where | Default | GDD |
|---|---|---|---|
| Footstep radius/intensity per tier | `DA_SoundProfile` | 150/0.10 · 700/0.35 · 2000/0.80 | §6.1 |
| Throw release vs impact | `DA_SoundProfile` | 900/0.40 vs 2500/0.90 | §6.3 |
| Gunshot | `DA_SoundProfile` | 9000/1.0 (map-wide-ish) | §6.1 |
| Occlusion factor / per-extra-surface | `DA_SoundProfile` | 0.35 / 0.6 | §8, §10 |
| Blackout clock | `BP_GM_Blackout.RoundSeconds` | 270 s | §7.1 |
| Blackout ammo | `BaseAmmo + Hiders/HidersPerExtraBullet` | 2 + n/3 | §7.1 |
| Blackout pot | `Scoring.BasePool × (1 + RiskBonus × caught/total)` | 1000, 1.0 | §7.1 |
| Manhunt ammo | `Hiders + ExtraAmmo` | n + 2 | §7.2 |
| Manhunt payouts | `SurvivorReward / ConsolationMax / HunterPointsPerCatch` | 1000 / 300 / 150 | §7.2 |
| Head start / results / spectate delay | `BSGameMode` | 15 / 12 / 6 s | §5, §11.2 |
| Throwables carried | `MaxThrowablesCarried` | 2 | §6.3 |
| Pulse cooldown / radius / reveal | `BSGA_Pulse` | 18 s / 1800 cm / 1.2 s | §4.1 |

---

## 6. Testing locally
PIE: Net Mode **Play As Listen Server**, Number of Players 4+. `MinPlayersToStart` (Project Settings › Blind
Sight) gates auto-start; or press `~` and run `BSStartRound`. Cyan/orange debug spheres are the Hunter's pings
until the HUD draws them.

---

## 7. Open items intentionally not built (GDD §11 / §13 stretch)
Voice chat policy, spectator polish beyond delayed Hunter-POV, cosmetics/progression, onboarding ammo assist,
anti-griefing heuristics, additional maps. `BSGameMode::HandleHiderEliminated` and `ResolveScoring` are the
seams to hook analytics for the playtest & balance plan.

---

## 8. Engine version

Targets **UE 5.6** (tested against 5.6.1). The 5.4 → 5.6 migration applied to this scaffold:

| Change | Where | Why |
|---|---|---|
| `EngineAssociation` → `5.6` | `BlindSight.uproject` | Binds the project to the 5.6 install |
| `IncludeOrderVersion` → `Unreal5_6` | both `*.Target.cs` | 5.6 reordered engine includes |
| `CppStandard = Cpp20` stated explicitly | both `*.Target.cs` | 5.6 dropped C++17 support |
| `WindowsPlatform.bStrictConformanceMode = true` | both `*.Target.cs` | Now the default; stated so it can't change silently |
| `AbilityTags.AddTag(...)` → `SetAssetTags(FGameplayTagContainer(...))` | `BSGA_Fire/Throw/Pulse.cpp` | `AbilityTags` deprecated in 5.5; asset tags are now constructor-set and read via `GetAssetTags()` |

`DefaultBuildSettings` stays at `BuildSettingsVersion.V5` — V6 arrives with 5.7, not 5.6.

Unchanged and still correct on 5.6: `SetNetUpdateFrequency()` (5.5+ accessor), `AddLooseGameplayTag` /
`RemoveLooseGameplayTag` (the `EGameplayTagReplicationState` overloads land in 5.7), native gameplay tag macros,
`ApplyModToAttribute`, `Online/OnlineSessionNames.h`, and the MetaSounds and OnlineSubsystemEOS module names.

**If you later move to 5.7**, the two known edits are `BuildSettingsVersion.V6` + `EngineIncludeOrderVersion.Unreal5_7`,
and the loose-tag calls in `BSCharacterBase`, `BSHiderCharacter` and `BSPlayerState` need the new replication-state
overloads.

---

## 9. Online backend

The game was already fully networked and server-authoritative. This section adds what it did
not have: a **dedicated server target**, and a **backend service** that owns identity,
progression, matchmaking and telemetry.

### 9.1 Why dedicated servers matter unusually much here

In a listen-server match the host machine *is* the server, so it holds every player's position
and every sound ping. In most games that is a mild integrity concern. Blind Sight is built
entirely on an information asymmetry — the Hunter is supposed to know only what they hear — so a
Hider who happens to be hosting has, in principle, perfect knowledge. The core fantasy breaks.
That is the argument for dedicated servers at playtest scale, not just at launch scale.

### 9.2 The security boundary

One rule governs the whole design: **clients can never write progression.**

| | Client (`UBSBackendSubsystem`) | Dedicated server (`UBSServerBackend`) |
|---|---|---|
| Auth | player JWT from `/auth/login` | `X-Server-Key` header |
| Can read own profile | yes | n/a |
| Can queue for a match | yes | n/a |
| Can equip owned cosmetics | yes (ownership re-checked) | n/a |
| Can buy cosmetics | yes (price and balance checked in a locked transaction) | n/a |
| **Can write points, XP, currency, stats** | **no — no such endpoint exists** | yes |
| Can post telemetry | no | yes |

`UBSServerBackend::ShouldCreateSubsystem` returns false in packaged non-server builds, so the
server key cannot be present in a client build. The key itself comes from `BS_SERVER_KEY` in the
environment, never a config file.

Round results are **idempotent** — `match_rounds` has `UNIQUE (match_id, round_number)` — so a
server retrying after a network blip cannot pay a round out twice. Point values are also clamped
in the request schema, so a bug in the game server (or a leaked key) has a bounded blast radius.

### 9.3 What runs where

```
UE client  ──auth/queue/profile──>  Backend (Node + Postgres)
                                      │  matchmaker loop every 2s:
                                      │  group queued tickets by (mode, region),
                                      │  allocate an idle heartbeating server
                                      v
UE client  ──ClientTravel(ip:port?MatchId=…)──>  Dedicated server (UE, headless)
                                                   │ validates each joiner against the match
                                                   │ runs rounds exactly as before
                                                   └─round results + telemetry──> Backend
```

### 9.4 Running the whole stack locally

```bash
docker compose up -d db backend
docker compose run --rm backend node dist/migrate.js
# after cooking a Linux server build into ./LinuxServer:
docker compose up -d gameserver-1 gameserver-2
```

The backend starts in **dev auth mode** when no identity provider is configured: it trusts the
id token as a raw account id. That is fine on a laptop and catastrophic in production — it is
printed as a warning on every boot for exactly that reason.

### 9.5 Cooking the Linux server build

```bash
RunUAT.sh BuildCookRun -project=/abs/path/BlindSight.uproject \
  -noP4 -platform=Linux -server -serverplatform=Linux -noclient \
  -cook -build -stage -pak -archive -archivedirectory=/abs/path/out
```

Copy the staged `LinuxServer` folder next to `Server/Dockerfile`, then
`docker build -f Server/Dockerfile -t blindsight-server:0.1 .`

On Windows you will need the **Linux cross-compile toolchain** matching 5.6 installed, and
`LINUX_MULTIARCH_ROOT` set.

### 9.6 Telemetry already wired

The game server records these without further work, which is what the GDD §11 tuning plan needs:

| Event | Fires | Carries |
|---|---|---|
| `round_start` | head start begins | round, hider count, hunter ammo |
| `hider_eliminated` | on each catch | placement, remaining, survival seconds |
| `round_end` | round resolves | end reason, survivors, duration |

Events are batched in memory, flushed every 20s and at match end, and capped at 500 buffered so
an unreachable backend cannot grow memory without bound. Add new events with one call to
`ABSGameMode::RecordTelemetry` — the payload is JSONB, so no migration is needed.

### 9.7 Still to wire (needs accounts you have to create)

- **EOS / Steam identity verification** — `resolvePlatformAccount()` in `Backend/src/auth.ts` has
  the two TODOs. Until then, dev auth mode.
- **Easy Anti-Cheat** — ships with EOS. Enable the plugin, register the product in the Epic
  portal, and run the client through the EAC bootstrapper. Dedicated servers need the EAC server
  module alongside the game binary.
- **Voice chat** — EOS RTC is the natural fit since you are already on EOS. Note GDD §11.1 is an
  open design question: free voice may trivialise the Hunter. Proximity-limited voice is the
  conservative default and matches the game's information design.
- **Hosting** — the container is orchestrator-agnostic. Agones on Kubernetes, AWS GameLift,
  Edgegap and Hathora all work; each injects the public address differently, which is the one
  thing `entrypoint.sh` needs adapting for.
