# statcpp CLI Design & Developer Guide

## Concept

**Bring statistics to the UNIX pipeline.**

Just as `awk` handles text processing and `jq` handles JSON processing, `statcpp` handles statistical processing.
A single binary, zero dependencies, fast startup — a tool that fits naturally into data analysis workflows.

**In a nutshell:** "The simplicity of datamash" + "The functionality of R" + "The speed of C++"

---

## Architecture

### File Structure

```
statcppCLI/
├── CMakeLists.txt                     Build configuration
├── cmake/
│   ├── gflags.cmake                   gflags auto-download + build
│   └── nlohmann-json.cmake            nlohmann/json auto-download
├── src/
│   ├── main.cpp                       Entry point (gflags definitions, dispatch)
│   └── include/
│       ├── csv_reader.hpp             CSV/TSV reader (RFC 4180)
│       ├── cli_parser.hpp             Subcommand parsing & shortcuts
│       ├── output_formatter.hpp       Text/JSON/quiet output
│       └── commands/
│           ├── desc.hpp               Descriptive statistics (17 commands)
│           ├── test_cmd.hpp           Statistical tests (12 commands)
│           ├── corr.hpp               Correlation & covariance (5 commands)
│           ├── effect.hpp             Effect size (6 commands)
│           ├── ci.hpp                 Confidence intervals (5 commands)
│           ├── reg.hpp                Regression analysis (5 commands)
│           ├── anova.hpp              Analysis of variance (5 commands)
│           ├── resample.hpp           Resampling (6 commands)
│           ├── ts.hpp                 Time series analysis (8 commands)
│           ├── robust.hpp             Robust statistics (7 commands)
│           ├── survival.hpp           Survival analysis (3 commands)
│           ├── cluster.hpp            Clustering (3 commands)
│           ├── multiple.hpp           Multiple testing correction (3 commands)
│           ├── power.hpp              Power analysis (3 commands)
│           ├── glm_cmd.hpp            Generalized linear models (2 commands)
│           └── model.hpp              Model selection (4 commands)
├── test/
│   ├── test_csv_reader.cpp            CSV reader unit tests
│   ├── test_output_formatter.cpp      Output formatter unit tests
│   ├── test_cli_parser.cpp            Argument parsing tests
│   └── e2e/
│       ├── run_e2e.sh                 E2E test runner
│       ├── data/                      Test CSV files
│       └── golden/                    Expected output files
└── download/                          Auto-generated (.gitignore target)
    ├── statcpp/
    │   ├── statcpp-main.tar.gz        statcpp archive cache
    │   └── statcpp-install/           statcpp headers (include/statcpp/)
    ├── gflags/
    │   ├── gflags/                    gflags source + build (_build/)
    │   └── gflags-install/            gflags install destination (lib/, include/)
    └── nlohmann-json/
        └── nlohmann-json-install/     json.hpp (include/nlohmann/)
```

### Header-Only Design

All command files are implemented as `.hpp` (header-only). Each function is marked `inline`.

**Rationale:**

1. The statcpp library itself is header-only — consistency of style
2. No need to add source files to CMakeLists.txt
3. `main.cpp` simply `#include`s each `.hpp` and everything is self-contained

**Trade-offs:**

- Compilation reduces to a single `main.cpp` file (not a problem currently)
- Build time may increase if the number of commands grows significantly

### Processing Flow

```
main.cpp
  ├── gflags::ParseCommandLineFlags()    Flag parsing
  ├── parse_command()                     Get category/command/file path
  ├── CsvReader::read_file/read_stdin()   Read CSV (except power/ci sample-size)
  ├── OutputFormatter(mode)               Determine output mode
  └── run_<category>()                    Dispatch by category
        ├── csv.get_clean_data()          Get column + remove missing values
        ├── statcpp::*()                  Statistical computation
        └── fmt.print() / fmt.flush()    Output results
```

---

## Dependencies

| Library | Purpose | Installation Method |
|---|---|---|
| [statcpp](https://github.com/mitsuruk/statcpp) | Core statistical computation (header-only) | Auto-downloaded from GitHub via `cmake/statcpp.cmake` |
| [gflags](https://github.com/gflags/gflags) | Command-line argument parsing | Auto-downloaded + built via `cmake/gflags.cmake` |
| [nlohmann/json](https://github.com/nlohmann/json) | JSON output | Auto-downloaded via `cmake/nlohmann-json.cmake` |
| [Google Test](https://github.com/google/googletest) | Unit testing (optional) | Enabled with `-DGTEST=true` |

### gflags Design

- Global flags (`--col`, `--json`, `--alpha`, etc.) are defined in `main.cpp` using `DEFINE_*` macros
- Each command file uses `DECLARE_*` to reference them
- Subcommands (category, command) are parsed manually in `cli_parser.hpp`, not by gflags

### Commands That Don't Require CSV

The following commands operate without CSV input:

- `ci sample-size` — Required sample size calculation
- `ci prop` — Confidence interval for proportions (specified with `--successes`, `--trials`)
- `effect cohens-h` — Effect size for proportions (specified with `--p1`, `--p2`)
- `power *` — All power analysis commands

Controlled by the `needs_csv` flag in `main.cpp`.

---

## Data Ordering & Sorting Strategy

### Sorting Requirements of the statcpp Library

statcpp contains a mix of **functions that require pre-sorted data** and **functions that sort internally**.
The CLI layer shields users from having to be aware of this difference.

| Category | Example Functions | CLI Handling |
|---|---|---|
| Requires sorted input | `median()`, `quartiles()`, `percentile()`, `iqr()`, `five_number_summary()` | Auto-sorted by CLI |
| Sorts internally | `mad()`, `shapiro_wilk_test()`, `kaplan_meier()` | Passed as-is |
| Order is meaningful (no sorting) | `acf()`, `moving_average()`, `diff()`, `t_test_paired()`, all regression | Passed as-is |

### Implementation Strategy

1. **Default:** Auto-sort on the CLI side as needed, based on each command's requirements
2. **Optimization:** `--presorted` skips the copy and sort for pre-sorted data
3. **summary command:** Sorts once and reuses for median, quartiles, five_number_summary
4. **Multiple columns:** Never sorted (would break the correspondence between columns)

### Order of Missing Value Removal and Sorting

```
1. Read CSV
2. Remove missing values (--skip_na)
3. Sort (only when necessary)
4. Statistical computation
```

---

## Output Design

### Three Output Modes

| Mode | Flag | Use Case | Format |
|---|---|---|---|
| Text | (default) | Human-readable | `Label:  value` |
| JSON | `--json` | Programmatic access | Structured JSON |
| Quiet | `--quiet` | Pipelines | Numeric values only |

### JSON Output Structure

```json
{
    "command": "desc.summary",
    "input": {
        "column": "value",
        "n": 5
    },
    "result": {
        "Count": 5.0,
        "Mean": 30.0,
        "Std Dev": 15.811388300841896,
        "Min": 10.0,
        "Median": 30.0,
        "Max": 50.0
    }
}
```

---

## Testing Strategy

### Terminology

| Term | Full Name | Meaning |
| --- | --- | --- |
| E2E test | End-to-End test | A test that verifies the entire system from input to output in one pass. While unit tests verify correctness at the function level, E2E tests execute the actual binary to confirm that "the same operations a user would perform produce the expected output" |
| Golden file | Golden file | An expected output file saved in advance as the "correct answer". During testing, the actual output is compared using `diff`, and any mismatch results in a FAIL. The name derives from "gold standard" |

### Three-Layer Structure

```
Layer 1: statcpp library tests (existing, no changes needed)
  ├── Google Test 758 tests (function-level correctness)
  └── R verification 167 checks (numerical precision guarantee)

Layer 2: CLI-specific unit tests (28 tests)
  ├── test_csv_reader.cpp      CSV/TSV parser tests
  ├── test_output_formatter.cpp Output formatting tests
  └── test_cli_parser.cpp      Argument parsing tests

Layer 3: E2E tests (52 tests)
  ├── Golden file tests         diff comparison against expected output
  └── Error case tests          Verification of error handling behavior

Layer 4: Reference verification (126 tests)
  ├── docs/run_reference.sh    Runs all examples from test-reference.md
  └── docs/output.txt          Execution results (PASS: 126, SKIP: 0)
```

### Build, Install & Run Tests

```bash
# Build the CLI binary
cmake -B build && cmake --build build

# Install (default: /usr/local/bin/statcpp)
sudo cmake --install build

# Install to a custom directory
cmake --install build --prefix ~/.local    # → ~/.local/bin/statcpp

# Unit tests (build with GTest enabled)
cmake -B build -DGTEST=true && cmake --build build && ctest --test-dir build --verbose

# Switch back to CLI binary (disable GTest)
cmake -B build -DGTEST=false && cmake --build build

# E2E tests
cd test/e2e && bash run_e2e.sh

# Verify all reference examples
bash docs/run_reference.sh
```

**Note:** When built with `-DGTEST=true`, the binary becomes a test runner.
To use it as a CLI tool, rebuild with `-DGTEST=false`.

### Updating Golden Files

When you change the output format:

1. Rebuild the CLI binary
2. Run the changed command and save the output to the golden file
3. Review the changes with `diff`
4. Run the full E2E test suite to verify

```bash
# Example: update the golden file for desc summary
cd test/e2e
../../build/statcpp desc summary data/basic.csv --col value > golden/desc_summary.txt
bash run_e2e.sh
```

---

## Adding a New Command

### 1. Create/Edit the Command File

Add a `cmd.command == "new-cmd"` branch to `src/include/commands/<category>.hpp`.

```cpp
} else if (cmd.command == "new-cmd") {
    auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
    double result = statcpp::new_function(data.begin(), data.end());
    fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
    fmt.print("Result", result);
}
```

### 2. For a New Category

1. Create `commands/new_category.hpp`
2. Add `#include` and `DECLARE_*` to `main.cpp`
3. Add an `else if` branch to the dispatch in `main.cpp`
4. Add to the `categories` vector in `cli_parser.hpp`
5. If the command doesn't require CSV, update the `needs_csv` condition
6. If new gflags are needed, add `DEFINE_*` to `main.cpp`

### 3. Add Tests

1. E2E test: Add a test case to `run_e2e.sh`
2. Golden file: Save expected output in `test/e2e/golden/`
3. Test data: Add CSV files to `test/e2e/data/` if needed

---

## Pipeline Usage Examples

```bash
# Summary statistics for a specific column in a CSV
statcpp desc summary data.csv --col price

# Normality test → choose the appropriate test
statcpp test shapiro data.csv --col score

# Pipe JSON output for processing
statcpp test t data.csv --col a,b --json | jq '.result["p-value"]'

# Pipe numeric-only output
statcpp desc mean data.csv --col price --quiet | xargs echo "Mean:"

# Read from stdin
cat data.csv | statcpp desc mean --col value
awk '{print $3}' access.log | statcpp desc summary --noheader --col 1

# --row: process inline data directly (comma or space-delimited)
echo "1,2,3,4,5" | statcpp desc mean --noheader --col 1 --row
echo "1 2 3 4 5" | statcpp desc mean --noheader --col 1 --row

# Batch analysis of multiple files
for f in experiment_*.csv; do
  echo "=== $f ==="
  statcpp desc summary "$f" --col result --quiet
done
```

---

## Unimplemented Design Ideas

The following features are described in `doc/CLI.md` (initial design document) but are not yet implemented:

- `--csv` output mode
- `--seed` random seed (for resampling)
- `--verbose` detailed output (e.g., sort time display)
- `test prop` / `test prop2` (proportion tests)
- `test chisq-indep` (chi-squared test of independence)
- `test fisher` (Fisher's exact test)
- `anova twoway` (two-way ANOVA)
- `anova ancova` (ANCOVA)
- `cluster` `--k` option (currently fixed to default k=3)
- `ts` `--lag`, `--window`, `--alpha` options (currently fixed to defaults)
- Shell completion (bash / zsh / fish)
- Man page generation
- Homebrew formula / apt package
