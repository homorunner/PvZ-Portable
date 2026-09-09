# In-Game Unit Tests

Run `pvz-portable.exe -unittest` with the usual game resources available
(`-resdir` is supported). No clicks are required. Results appear at the top
left and in `unittest.log` beside the executable, overwritten for each run.
After the last case the final scene is frozen for five wall-clock seconds,
then the game shuts down. Exit status is 0 for a completed passing suite,
1 for a failed or interrupted suite.

All profile and registry I/O uses a newly created temporary sandbox, even
if `-savedir` is supplied. The sandbox is removed after game destruction.
Automatic mid-level saving is disabled; explicit serialization tests write
only inside that sandbox. Tests bypass menus, cutscenes, wave spawning,
level progression and board input, but run real game-object updates,
projectile collisions and effects at the normal 10 ms simulation tick.

Add cases to `RegisterLawnTests` in `UnitTestCases.cpp`. Each registry entry
has setup, post-update and optional projectile-created callbacks, plus a
tick timeout. Setup must create a fresh board and reset its case state.
Call `Check` for assertions and `Finish` when observation is complete.
Failures are accumulated and subsequent cases still run serially.
`Finish` only records the result. Case transitions and shutdown run in
`AfterFrame`, called by `LawnApp::UpdateFrames` after widget updates return;
callbacks must not replace the board or shut down from a board update.
Projectile callbacks occur immediately after allocation, before the firing
plant sets motion, so inspect motion in the post-update callback instead.
Use object IDs rather than retaining pointers across deletion queues.

The leftpeater burst case uses one-based row 3 / column 8 for a stationary normal
zombie and row 3 / column 9 for the leftpeater. Current and maximum health
are both 80; normal headless decay is allowed after shot three. It verifies
leftward peas at ticks 0, 17, 33, 50, damage and fourth-shot death, and keeps
observing through tick 140 to reject extra burst shots. It also checks the
shooting animation and launch-counter reset to 120 at tick 50, followed by
the normal decrement to 119 at tick 51, resolving the plant by ID.

The disabled burst case sets the runtime global `ENABLE_LEFTPEATER_PLANTING_BURST`
to false before planting. It checks that no bonus shot or animation is
scheduled, then fixes the random initial attack counter at 150 for a
repeatable normal-attack test. At tick 50 the target must still have 80
health and the counter must be 100. Normal shots must occur at ticks 150
and 175, leaving the target alive with 40 health at tick 200. The switch
defaults to true and is restored after this case.

The Plantern healing cases run with `ENABLE_PLANTERN_HEALING` enabled and
disabled (default true), restoring its prior value afterward. A damaged
Plantern starts with launch counter 100, surrounded by eight damaged
wallnuts, with a pumpkin in its own cell and a wallnut two columns away.
Every simulation tick through 200 checks health and the healing timer:
only the eight neighbors gain 45 HP at ticks 100 and 200, never earlier;
one neighbor starts just 10 HP below maximum to check the cap. Neither the
Plantern, its same-cell pumpkin nor the distant plant is healed. With the
switch disabled, all health and the unused timer remain unchanged.

The green-vase cases use real endless `ScaryPotterPopulate` calls at stages
0, 1, 8 and 9 with `ENABLE_PLANTERN_GREEN_VASE` enabled and disabled (default
true), restoring its prior value afterward. Each population is isolated
by clearing the previous grid items. Assertions check 35 uniquely placed
vases, exactly two green seed vases, the complete original seed inventory,
one valid sun vase, five bucketheads, one jack-in-the-box, and difficulty
counts of `1 + min(stage, 8)` Gargantuars and `8 - min(stage, 8)` normals.
When enabled, the single existing Plantern must be green. When disabled,
its color is deliberately unconstrained: random selection may still make
it green, so the test does not assert a chance outcome.

Synchronous setup-only cases may call `Finish` before returning, as the
vase cases do. Their post-update callback can be null because the runner
skips updates for finished cases; transitions still occur in `AfterFrame`.

## Empowered Peashooter

`ENABLE_PEASHOOTER_EMPOWERED_PEA` in `GameConstants.h` is an independent,
mutable inline bool, default true. Only plants whose actual `mSeedType` is
`SEED_PEASHOOTER` qualify (including an Imitater after it becomes that type).
Each plant counts emitted projectiles modulo three, not attack attempts or
hits. Shots 3, 6, 9, ... snapshot the toggle at emission. Disabled shots still
advance cadence; changing the toggle does not alter projectiles in flight.
New/reinitialized plants start at zero, including recycled allocation slots.

An empowered pea draws at 1.5x scale with unchanged collision dimensions,
speed, and targeting. Direct damage is 30 instead of 20. Surviving targets
are displaced opposite their walking direction by 11.525217 pixels; both
Gargantuar and red-eye Giga receive half, 5.762609 pixels. Integer collision
X is synchronized immediately. Dead/dying targets are not displaced.

Torchwood retains the projectile's empowerment, scaling its fire animation
1.5x and direct fire damage from 40 to 60. Fire splash damage and its radius
stay unchanged, and splash-only targets receive no knockback. Snow peas
converted into ordinary peas do not acquire empowerment.

Fire animation scaling uses the nominal local pivot `(40, 40)`: the loaded
38x38 body begins at `(20.8, 19.9)`, near that center, and the original
80-pixel backward mirror correction fixes X=40. Reanimation overlay scaling
acts about the origin, so offsets are `-25 + 40 * (1 - scale)` on both axes;
backward shots additionally add `80 * scale` to X. The world pivot remains
`projectile position + (15, 15)`. At 1.5x, forward offsets are `(-45, -45)`
and backward offsets are `(75, -45)`; 1x offsets remain unchanged.
The fire pea alignment case checks both scales and directions immediately
and after attachment movement, plus the unchanged 30x40 fireball collision
rectangle. It logs loaded resource track metadata for pivot evidence.

### Movement Evidence

Normal `Zombie::PickRandomSpeed` chooses uniformly from 0.23 to 0.37, mean
0.30. However, `UpdateZombieWalking` uses
`GetTrackVelocity("_ground") * mScaleZombie`, not that velocity directly.
`UpdateAnimSpeed` sets animation rate to `mVelX * N / D * 47 / scale`, where
`N` is the walk frame count and `D` its total ground displacement.
`Reanimation::GetTrackVelocity` uses adjacent ground displacement times
`SECONDS_PER_UPDATE` (0.01) times the animation rate. Ordinary `REANIM_LOOP`
samples `N - 1` intervals in `GetFrameTime`. Thus the full-gait mean is:

```text
mean speed = (D / (N - 1)) * (0.30 * N / D * 47) = 0.30 * 47 * N/(N - 1)
N = 47 for both loaded normal walk animations
mean speed = 14.406522 pixels/second
knockback = 80% * 14.406522 = 11.525217 pixels
Gargantuar/Giga knockback = 5.762609 pixels
```

The walking-evidence case samples 10,000 evenly spaced gait phases through
real `UpdateZombieWalking` calls at mean speed. With the current resources,
`anim_walk` measures 14.408191 px/s and `anim_walk2` 14.403781 px/s (float
position rounding and sampling account for the small difference). This is a
fixed normal-zombie baseline, not a target's chilled or instantaneous speed.

### Coverage And Saves

Enabled and disabled integration cases run five staggered normal Peashooters
for 1,050 simulation ticks, observing six real emitted projectiles per plant
and all resulting collisions. They check per-plant third/sixth-shot cadence,
visual-scale selection, unchanged collision sizes, aggregate damage, normal
knockback, both giants' half knockback, and real Torchwood conversion.

A setup-only case round-trips cadence and in-flight empowerment through
`LawnSaveGame`/`LawnLoadGame` in the sandbox, resumes the sixth shot, checks
runtime disabling and recycled projectile slots, and excludes Repeater,
Leftpeater, Threepeater, Gatling Pea and Snow Pea. It also hides the new save
fields as unknown TLVs, recomputes the CRC and reloads, verifying absent-field
defaults and unknown-field skipping without losing entities.

Portable saves keep their existing versions and positional tails. Optional
entity field 101 stores the plant counter or projectile flag; older portable
saves default them to zero/false. Older readers skip this optional field,
so loading and resaving there loses empowerment state. Legacy raw ABI loading
retains the original entity sizes and ID offsets, explicitly clearing new
members after copying old bytes (which may include overlapping tail padding).
No user saves or profiles are touched by tests. Historical raw-save fixtures
and pixel-level visual comparisons are not covered by this runner.

### Windows Verification

From the repository root, use the installed VS developer shell and existing
build directory:

```powershell
Test-Path -LiteralPath 'D:\games\PvZ-Portable\build-msvc'
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\Launch-VsDevShell.ps1' -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
cmake --build 'D:\games\PvZ-Portable\build-msvc' --parallel 4
if (!(Test-Path -LiteralPath 'D:\games\PvZ-Portable\build-msvc')) { throw 'Missing test directory' }
$p = Start-Process -FilePath 'D:\games\PvZ-Portable\build-msvc\pvz-portable.exe' -ArgumentList '-unittest' -WorkingDirectory 'D:\games\PvZ-Portable\build-msvc' -PassThru
if (!$p.WaitForExit(120000)) { Stop-Process -Id $p.Id; throw 'Unit tests timed out' }
$p.Refresh()
$p.ExitCode
```

Verified with the existing MSVC build: 11 cases passed, 0 failed, 5,034
checks; process exit code 0. Results are in `build-msvc/unittest.log`.
