# Save format

The foundation writes SQLite databases with `PRAGMA user_version = 1`. The `snapshot` table has a single row (`id=1`): `tick` and `rng` are eight-byte little-endian unsigned integer blobs; `credits` is a nonnegative SQLite integer. Event subscriptions are not saved. RNG state is persisted so subsequent ticks reproduce the original run. A save is replaced inside an immediate transaction. Loading uses a read transaction, validates the schema and fields, and only then replaces simulation state. Unknown versions are rejected; an unversioned, nonempty database is never adopted as a save.

This is a prototype schema, not a promised stable player-facing format. There are no prior versions to migrate yet. Future schema changes must add explicit migration and regression fixtures. Autosave rotation, backup recovery and compression are not implemented.

## Back Room manual save

The playable scenario has a separate SQLite database identified by `application_id = 0x4e534247` and `user_version = 2`. It stores all game counters, rack temperature, outcome and the eight-byte little-endian RNG state in a single `game` row. Saves are written in an immediate transaction; loads validate types, bounds and outcome before changing the active run. The game never adopts an unrelated database. This is separate from the headless prototype schema above. Version 1 saves load with ambient temperature (24 C); the next save migrates them transactionally to version 2. Unknown versions are rejected. There is one manual slot and no autosave rotation yet.
