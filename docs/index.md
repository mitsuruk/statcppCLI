# statcpp CLI

**Bring statistics to your UNIX pipeline.**
*Also runs on Windows (MSVC / MinGW)*

## Overview

A command-line statistics tool built on the C++17 header-only statistics library `statcpp`.
Just as `awk` handles text processing and `jq` handles JSON, `statcpp` handles statistical analysis.
A single binary with zero dependencies and instant startup that fits naturally into your data analysis workflow.

### Features

- 16 categories, 94 commands (from descriptive statistics to survival analysis)
- Automatic CSV / TSV detection, stdin pipe support
- Text / JSON / quiet output modes
- C++17 single binary, startup in milliseconds

## Quick Start

```bash
# Display all basic statistics at once
statcpp desc summary data.csv --col price

# Mean (via pipe)
cat data.csv | statcpp desc mean --col value

# t-test (two-group comparison)
statcpp test t data.csv --col group1,group2

# JSON output
statcpp desc summary data.csv --col price --json

# Numeric value only (for piping)
statcpp desc mean data.csv --col price --quiet
```

### Output Example

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

## Documentation

- [Command Reference](commands.md) — Options and usage for all commands
- [Design Guide](design.md) — Architecture and development information
- [Test Reference](test-reference.md) — Execution examples and output verification

## License

MIT License
