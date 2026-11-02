# Save

SQLite persistence is separate from the pure simulation. A version 1 snapshot is written transactionally and restored only after validation. Paths use SQLite's UTF-8 interface. Backups, autosaves and migrations from future schemas remain unimplemented.
