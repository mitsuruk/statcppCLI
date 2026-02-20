# statcpp CLI

**Bring statistics to your UNIX pipeline.**

[日本語版 / Japanese](README-ja.md)

## Development Purpose

A command-line statistics tool built on the C++17 header-only statistics library `statcpp`.
Just as `awk` handles text processing and `jq` handles JSON, `statcpp` handles statistical analysis.
The goal is a single binary with zero dependencies and instant startup that fits naturally into your data analysis workflow.

## Features

- 16 categories, 94 commands (from descriptive statistics to survival analysis)
- Automatic CSV / TSV detection, stdin pipe support
- Text / JSON / quiet output modes
- C++17 single binary, startup in milliseconds

## Build

```bash
git clone <repository-url>
cd statcppCLI
cmake -B build && cmake --build build
```

Built binary: `build/statcpp`

### Install

```bash
# Install to /usr/local/bin/statcpp
sudo cmake --install build

# Install to a custom directory
cmake --install build --prefix ~/.local
```

### Prerequisites

- CMake 3.20+
- C++17 compatible compiler (GCC 9+, Clang 10+, Apple Clang 12+)

statcpp, gflags, and nlohmann/json are automatically downloaded by CMake.

## Quick Start

```bash
# Display all basic statistics at once
statcpp desc summary data.csv --col price

# Mean (via pipe)
cat data.csv | statcpp desc mean --col value

# Directly from stdin
printf '1\n2\n3\n4\n5\n' | statcpp desc mean --noheader --col 1

# Inline data (space or comma-delimited)
echo '1 2 3 4 5' | statcpp desc mean --noheader --col 1 --row

# t-test (two-group comparison)
statcpp test t data.csv --col group1,group2

# Correlation analysis
statcpp corr pearson data.csv --col x,y

# Regression analysis
statcpp reg simple data.csv --col x,y

# JSON output
statcpp desc summary data.csv --col price --json

# Numeric value only (for piping)
statcpp desc mean data.csv --col price --quiet

# Power analysis (no CSV required)
statcpp power t-one --effect 0.5 --n 30
```

## Output Example

```text
$ statcpp desc summary data.csv --col value
  Count:        5
  Mean:         30
  Std Dev:      15.8114
  Min:          10
  Q1:           20
  Median:       30
  Q3:           40
  Max:          50
  Skewness:     0
  Kurtosis:     -1.2
```

## Categories

| Category | Description | Commands |
| --- | --- | --- |
| `desc` | Descriptive statistics | 17 |
| `test` | Statistical tests | 12 |
| `corr` | Correlation and covariance | 5 |
| `effect` | Effect size | 6 |
| `ci` | Confidence intervals | 5 |
| `reg` | Regression analysis | 5 |
| `anova` | Analysis of variance | 5 |
| `resample` | Resampling | 6 |
| `ts` | Time series analysis | 8 |
| `robust` | Robust statistics | 7 |
| `survival` | Survival analysis | 3 |
| `cluster` | Clustering | 3 |
| `multiple` | Multiple testing correction | 3 |
| `power` | Power analysis | 3 |
| `glm` | Generalized linear models | 2 |
| `model` | Model selection | 4 |

Shortcuts: `mean`, `median`, `mode`, `sd`, `var`, `summary`, `range`, `iqr`, `cv`, `skewness`, `kurtosis`, `quartiles`, `gmean`, `hmean`, `ttest`, `pearson`, `spearman`, `kendall`

## Documentation

- [Command Reference](docs/commands.md) - Options and usage for all commands
- [Test Reference](docs/test-reference.md) - Execution examples and output verification for all commands
- [Design and Developer Guide](docs/design.md) - Architecture and development information
- [Test Verification Results](docs/output.txt) - Execution results for all commands

## Test

```bash
# Unit tests
cmake -B build -DGTEST=true && cmake --build build && ctest --test-dir build --verbose

# E2E tests
cd test/e2e && bash run_e2e.sh

# Verify all reference examples
bash docs/run_reference.sh
```

## Tested Environments

- macOS + Apple Clang 17.0.0
- macOS + GCC 15 (Homebrew)
- Ubuntu 24.04 ARM64 + GCC 13.3.0

## License

This project is released under the MIT License. See the [LICENSE](LICENSE) file for details.

### Dependencies

| Library | License |
| --- | --- |
| [statcpp](https://github.com/mitsuruk/statcpp) | MIT License |
| [gflags](https://github.com/gflags/gflags) | BSD 3-Clause License |
| [nlohmann/json](https://github.com/nlohmann/json) | MIT License |

## Acknowledgments

This project was developed using the following tools and AI:

- **Claude Code for VS Code Opus 4.6** - Terminology consistency in documentation, identifying missing explanations, test code generation and result verification
- **OpenAI ChatGPT 5.2** - Terminology consistency in documentation, identifying missing explanations

---

**Note**: This software does not have the same level of numerical stability or edge case handling as commercial statistical software. When using for research or production, we recommend verifying results with other tools.
