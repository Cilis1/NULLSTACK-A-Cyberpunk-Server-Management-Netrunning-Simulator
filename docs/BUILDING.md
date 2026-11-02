# Building

Install CMake 3.25 or newer and a C++20 toolchain. Use the presets in the README; the default generator is chosen by CMake. For other generators, configure with `cmake -S . -B build/custom -DBUILD_TESTING=ON`, build with `cmake --build build/custom`, and test with `ctest --test-dir build/custom --output-on-failure` (add `-C Release` for multi-config generators). No package manager is required for the current dependency-free core. vcpkg and FetchContent integration will be introduced alongside actual dependencies, rather than presenting untested configuration as supported.
