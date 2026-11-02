# Core

Platform-independent simulation and deterministic primitives. No rendering or operating-system dependencies. Replay uses absolute tick numbers and applies commands before advancing each tick. Schedule validation occurs before execution; a command failure stops replay at that command and does not roll back earlier commands. Event handlers run synchronously and must not change subscriptions during publication.

Replay logs are UTF-8/ASCII text with `NULLSTACK_REPLAY 1`, `seed N`, `ticks N`, then zero or more `command T add|spend N` lines. Integers are decimal, commands are ordered, and the parser limits lines to 256 bytes and logs to 100,000 commands. File formats are prototypes, not yet stable compatibility promises.
