# In-Game Unit Tests

Run `pvz-portable.exe -unittest` with the usual game resources available
(`-resdir` is supported). No clicks are required. Results appear at the top
left and in `unittest.log` beside the executable, overwritten for each run.
After the last case the final scene is frozen for five wall-clock seconds,
then the game shuts down. Exit status is 0 for a completed passing suite,
1 for a failed or interrupted suite.

All profile and registry I/O uses a newly created temporary sandbox, even
if `-savedir` is supplied. The sandbox is removed after game destruction.
Mid-level saving is disabled. Tests bypass menus, cutscenes, wave spawning,
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

The initial case uses one-based row 3 / column 8 for a stationary normal
zombie and row 3 / column 9 for the leftpeater. Current and maximum health
are both 80; normal headless decay is allowed after shot three. It verifies
leftward peas at ticks 0, 17, 33, 50, damage and fourth-shot death, and keeps
observing through tick 140 to reject extra burst shots. It also checks the
shooting animation and launch-counter reset to 150 at tick 50, followed by
the normal decrement to 149 at tick 51, resolving the plant by ID.

The second case sets the runtime global `ENABLE_LEFTPEATER_PLANTING_BURST`
to false before planting. It checks that no bonus shot or animation is
scheduled, then fixes the random initial attack counter at 150 for a
repeatable normal-attack test. At tick 50 the target must still have 80
health and the counter must be 100. Normal shots must occur at ticks 150
and 175, leaving the target alive with 40 health at tick 200. The switch
defaults to true and is restored after this case.
