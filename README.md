# NULLSTACK

NULLSTACK is a cyberpunk server-management and netrunning game in development. **Back Room** is a small, playable early-access terminal scenario: grow a server rack, run hosting contracts or risky intrusions, balance heat and maintenance, and reach the victory target before a raid or bankruptcy. It is not the full game described in the [design](docs/DESIGN.md) and [roadmap](docs/ROADMAP.md).

## Play the early-access scenario

Download the platform archive from [Releases](https://github.com/Cilis1/NULLSTACK-A-Cyberpunk-Server-Management-Netrunning-Simulator/releases), on Windows, run `NullstackSetup.exe` to install the game. If the release is private, download the matching Windows ZIP in your browser first and select it when prompted; otherwise setup can download it itself. Alternatively extract the ZIP and launch `Nullstack.exe`. On Linux, extract the archive and run `./Nullstack` in a terminal. Enter `help` to see commands and the victory condition. You can pass an unsigned numeric seed on the command line to repeat a run. The scenario currently has **no save/resume**. The Windows binary is unsigned and may trigger SmartScreen. The basic installer has no uninstaller or self-updater; see [installer documentation](installer/bootstrapper/README.md).

## Build and test

Requires CMake 3.25+ and a C++20 compiler. SQLite is used for the separate headless prototype's saves; CMake finds a system installation or downloads a pinned amalgamation on first configure.

```sh
cmake --preset linux-debug
cmake --build --preset linux-debug
ctest --preset linux-debug
./build/linux-debug/Nullstack
./build/linux-debug/nullstack_headless --seed 42 --ticks 100000 --save run.sqlite
```

On Windows with Visual Studio: `cmake --preset windows-release`, `cmake --build --preset windows-release`, then `ctest --preset windows-release`. The headless runner is a separate architectural prototype; its saves and replay logs do not capture the playable scenario. See [BUILDING](docs/BUILDING.md) and [TESTING](docs/TESTING.md).

Code is MIT licensed. See [THIRD_PARTY.md](THIRD_PARTY.md) for dependency licenses.
