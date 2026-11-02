# Testing

`ctest --preset linux-debug` runs core assertions and a headless 100,000-tick smoke run. Tests check fixed PRNG vectors, command validation, ordering, event delivery and deterministic replay. Replays currently live in memory only. SQLite tests cover save/load round trips, continued determinism, replacement, invalid state and unknown schema rejection. This is not yet a coverage measurement, migration suite, sanitizer run, or 10-year economy test. CI exercises GCC, Clang and MSVC builds; check the actual workflow run before calling it green.
