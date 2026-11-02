# Headless runner

A minimal deterministic smoke runner for the core. It can save and resume SQLite snapshots with `--save` and `--load`, or record and play input logs with `--record` and `--replay`. Record currently captures the runner’s command list, not interactive inputs; the runner itself has no interactive command entry. It is not yet a playable game or a balancing harness.
