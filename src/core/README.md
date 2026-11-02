# Core

Platform-independent simulation and deterministic primitives. No rendering or operating-system dependencies. Replay uses absolute tick numbers and applies commands before advancing each tick. Schedule validation occurs before execution; a command failure stops replay at that command and does not roll back earlier commands. Event handlers run synchronously and must not change subscriptions during publication.
