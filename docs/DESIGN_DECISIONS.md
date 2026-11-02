# Design decisions

## Foundation before breadth
A small deterministic command-driven model is preferable to empty interfaces for every future subsystem. It is a test fixture, not a representation of the final economy.

## Stable reproducibility
The core uses fixed-width integer arithmetic and explicit PRNG and hash algorithms. Replay input order is significant and invalid input is rejected rather than silently repaired.

## Persistence boundary
SQLite is isolated in a persistence library and uses transactional snapshots; replay is a separate reproducibility tool. Schema changes require explicit migrations before a future schema is accepted.

## Licensing
MIT for code to minimize contribution and integration friction. No third-party art or font is bundled at this stage.

## Early-access playable slice
The user requested a runnable release even if incomplete. Back Room is a separate, deliberately small turn-based terminal scenario with explicit victory and loss conditions, built on the platform-independent core. Its rules are currently hardcoded to make the slice testable; this is an acknowledged exception to the planned data-driven game, not a claim that the content architecture is complete. Its state is separate from the prototype snapshot and replay systems; those do not yet save or replay gameplay.

## Windows windowed prototype
The early-access Windows build uses native Win32 controls for a runnable window without a console or additional runtime dependencies. This is a temporary presentation layer over the same deterministic rules, not the planned SDL/OpenGL/ImGui VOIDSHELL client. Linux continues to use the terminal interface.
