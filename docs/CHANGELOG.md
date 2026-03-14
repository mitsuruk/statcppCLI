# Changelog

This document records the change history of statcppCLI.

This project follows [Semantic Versioning](https://semver.org/).

## [0.2.0] - 2026-03-13

### Changed

- **`test_cmd.hpp` — `ks` subcommand**: Updated internal call from `statcpp::ks_test_normal()` to `statcpp::lilliefors_test()` to follow the upstream rename in statcpp. The CLI subcommand name `ks` is unchanged.

### Documentation

- **`test_cmd.hpp`**: Updated Doxygen comments from "Kolmogorov-Smirnov test" to "Lilliefors test".
- **`commands.md` / `commands-ja.md`**: Updated `ks` subcommand description from "Kolmogorov-Smirnov normality test" to "Lilliefors normality test".
- **`test-reference.md` / `test-reference-ja.md`**: Rewrote `ks` section to clarify that the function performs a Lilliefors test (parameters estimated from data), not a standard KS test with known parameters.

### Dependencies

- **statcpp**: v0.2.0
