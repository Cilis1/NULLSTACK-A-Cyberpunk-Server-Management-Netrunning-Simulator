# NULLSTACK

A cyberpunk server-management and netrunning simulator, at the **early foundation** stage. This repository currently contains a deterministic C++20 headless simulation prototype, not a playable game, graphical client, installer, or release. See [design](docs/DESIGN.md), [architecture](docs/ARCHITECTURE.md), and [roadmap](docs/ROADMAP.md).

## Build and test

Requires CMake 3.25+ and a C++20 compiler. SQLite is used for saves; CMake finds a system installation or downloads a pinned amalgamation on first configure.

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
./build/linux-debug/nullstack_headless --seed 42 --ticks 100000 --save run.sqlite
./build/linux-debug/nullstack_headless --load run.sqlite --ticks 100
```

On Windows with Visual Studio: `cmake --preset windows-release`, `cmake --build --preset windows-release`, then `ctest --preset windows-release`. The runner accepts `--help` and reports its seed-dependent state hash. Tests include a 100,000-tick replay comparison. See [BUILDING](docs/BUILDING.md) and [TESTING](docs/TESTING.md).

Code is MIT licensed. See [THIRD_PARTY.md](THIRD_PARTY.md) for dependency licenses.
