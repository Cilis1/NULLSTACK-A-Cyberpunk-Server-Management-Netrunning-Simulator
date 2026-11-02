# Architecture

The core is a C++20 static library with no windowing or rendering dependencies. The headless runner and future graphical client communicate with it through commands and observable state. Platform UI, audio and rendering must never affect simulation results.

Simulation advances in discrete ticks. Commands are applied in a documented order; a replay consists of a seed and ordered tick-stamped commands. Random streams must use explicitly specified integer operations, not implementation-dependent standard distributions. State hashing must serialize fields explicitly rather than hashing object memory. Save files are versioned and loaded transactionally; unknown versions must fail without replacing the live state.

The initial core provides IDs, results, an event bus, a deterministic PRNG, a small command-driven simulation, replay and an in-memory replay runner and a separate SQLite persistence boundary. This deliberately exercises architecture rather than pretending to implement game systems. Content and gameplay parameters will move to validated data definitions when actual systems are introduced. No SDL, ImGui or SQLite is linked into core.

Future parallel passes must compute from immutable snapshots and commit in a stable order. Mod scripts will run with restricted libraries and explicit permission grants; no sandbox is claimed until implemented and tested.
