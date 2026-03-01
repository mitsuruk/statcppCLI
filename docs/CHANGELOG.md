# Changelog

This document records the change history of statcppCLI.

This project follows [Semantic Versioning](https://semver.org/).

## [0.2.1] - 2026-03-01

### Changed

- **`test_cmd.hpp` — `ks` subcommand**: Updated internal call from `statcpp::ks_test_normal()` to `statcpp::lilliefors_test()` to follow the upstream rename in statcpp v0.1.3. The CLI subcommand name `ks` is unchanged.

### Documentation

- **`test_cmd.hpp`**: Updated Doxygen comments from "Kolmogorov-Smirnov test" to "Lilliefors test".
- **`commands.md` / `commands-ja.md`**: Updated `ks` subcommand description from "Kolmogorov-Smirnov normality test" to "Lilliefors normality test".
- **`test-reference.md` / `test-reference-ja.md`**: Rewrote `ks` section to clarify that the function performs a Lilliefors test (parameters estimated from data), not a standard KS test with known parameters.

### Dependencies

- **statcpp**: Updated from v0.1.2 to v0.1.3 (re-downloaded from GitHub `main` branch).

---

[0.2.1]: https://github.com/mitsuruk/statcppCLI/releases/tag/v0.2.1
