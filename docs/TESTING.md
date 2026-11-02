# Testing

`ctest --preset linux-debug` runs scenario victory, failure, command and terminal smoke tests as well as core assertions and a headless 100,000-tick smoke run. Tests check fixed PRNG vectors, command validation, ordering, event delivery and deterministic replay. Replay tests exercise the versioned log format, malformed input, 100,000-tick playback and the headless CLI. SQLite tests cover save/load round trips, continued determinism, replacement, invalid state and unknown schema rejection. This is not yet a coverage measurement, migration suite, sanitizer run, or 10-year economy test. CI exercises GCC, Clang and MSVC builds; check the actual workflow run before calling it green.

The Back Room save tests cover round trips, deterministic continuation, malformed states, unsupported schemas and unrelated databases. The terminal integration test exercises save and load commands.
