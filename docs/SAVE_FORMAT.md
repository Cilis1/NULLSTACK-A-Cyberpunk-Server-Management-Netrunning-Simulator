# Save format

The foundation writes SQLite databases with `PRAGMA user_version = 1`. The `snapshot` table has a single row (`id=1`): `tick` and `rng` are eight-byte little-endian unsigned integer blobs; `credits` is a nonnegative SQLite integer. Event subscriptions are not saved. RNG state is persisted so subsequent ticks reproduce the original run. A save is replaced inside an immediate transaction. Loading uses a read transaction, validates the schema and fields, and only then replaces simulation state. Unknown versions are rejected; an unversioned, nonempty database is never adopted as a save.

This is a prototype schema, not a promised stable player-facing format. There are no prior versions to migrate yet. Future schema changes must add explicit migration and regression fixtures. Autosave rotation, backup recovery, compression and user-facing save management are not implemented.
