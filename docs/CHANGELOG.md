# Changelog

This document records the change history of statcppCLI.

This project follows [Semantic Versioning](https://semver.org/).

## [0.3.0] - 2026-04-07

### Added

- **Windows (MSVC) support**: CMakeLists.txt and cmake/gflags.cmake updated for cross-platform compatibility.
  - Compiler flags split by `CXX_COMPILER_ID`: MSVC uses `/O2 /W4 /Od /Zi`, GCC/Clang keep existing flags.
  - Encoding flags split: MSVC uses `/utf-8`, GCC/Clang use `-finput-charset=UTF-8 -fexec-charset=UTF-8`.
  - gflags cache detection changed from `libgflags.a` (Unix-only) to `gflags-config.cmake` (cross-platform).
  - gflags sub-build now passes `-G` and `-A` to match the parent project's generator and platform.
- **CI: Windows build job**: Added `build-windows` job (`windows-latest` / MSVC) to `.github/workflows/ci.yml`.

### Documentation

- Platform badge updated: `macOS | Linux` → `macOS | Linux | Windows`.
- Added subtitle *Also runs on Windows (MSVC / MinGW)* to README, index, and design docs.
- Prerequisites updated to include MSVC 2019+.
- Windows build and install instructions added to README.
- E2E tests noted as macOS / Linux only (bash required); unit tests work on Windows.
- Tested Environments updated to include Windows 11 ARM64 + MSVC 2022.

## [0.2.0] - 2026-03-13

### Changed

- **`test_cmd.hpp` — `ks` subcommand**: Updated internal call from `statcpp::ks_test_normal()` to `statcpp::lilliefors_test()` to follow the upstream rename in statcpp. The CLI subcommand name `ks` is unchanged.

### Documentation

- **`test_cmd.hpp`**: Updated Doxygen comments from "Kolmogorov-Smirnov test" to "Lilliefors test".
- **`commands.md` / `commands-ja.md`**: Updated `ks` subcommand description from "Kolmogorov-Smirnov normality test" to "Lilliefors normality test".
- **`test-reference.md` / `test-reference-ja.md`**: Rewrote `ks` section to clarify that the function performs a Lilliefors test (parameters estimated from data), not a standard KS test with known parameters.

### Dependencies

- **statcpp**: v0.2.0
