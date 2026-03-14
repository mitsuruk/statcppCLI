/**
 * @file main.cpp
 * @brief Entry point for the statcpp command-line statistics tool.
 *
 * Parses command-line flags and subcommands, reads CSV input, and dispatches
 * to the appropriate statistical analysis command handler.
 */

#include <iostream>
#include <string>

#include <gflags/gflags.h>

#include "csv_reader.hpp"
#include "cli_parser.hpp"
#include "output_formatter.hpp"
#include "commands/desc.hpp"
#include "commands/test_cmd.hpp"
#include "commands/corr.hpp"
#include "commands/ci.hpp"
#include "commands/effect.hpp"
#include "commands/reg.hpp"
#include "commands/anova.hpp"
#include "commands/resample.hpp"
#include "commands/ts.hpp"
#include "commands/robust.hpp"
#include "commands/survival.hpp"
#include "commands/cluster.hpp"
#include "commands/multiple.hpp"
#include "commands/power.hpp"
#include "commands/glm_cmd.hpp"
#include "commands/model.hpp"

/// @name Global flags - Descriptive statistics
/// @{
DEFINE_string(col, "", "Column name or 1-based index (comma-separated for multiple)");
DEFINE_string(delimiter, "", "CSV delimiter (auto-detect if empty)");
DEFINE_bool(header, true, "CSV has header row");
DEFINE_bool(json, false, "Output in JSON format");
DEFINE_bool(quiet, false, "Minimal output (numbers only)");
DEFINE_double(alpha, 0.05, "Significance level");
DEFINE_double(level, 0.95, "Confidence level");
DEFINE_bool(population, false, "Use population statistics (ddof=0)");
DEFINE_string(na, "NA,NaN,nan,N/A,n/a", "Missing value strings");
DEFINE_bool(skip_na, true, "Skip missing values");
DEFINE_bool(fail_na, false, "Error if missing values present");
DEFINE_bool(presorted, false, "Data is already sorted");
DEFINE_double(p, 0.5, "Percentile value (0.0-1.0)");
DEFINE_double(trim, 0.1, "Trim proportion for trimmed mean");
DEFINE_bool(row, false, "Treat single-line input as row data (comma/space separated)");
/// @}

/// @name Global flags - Statistical tests
/// @{
DEFINE_bool(paired, false, "Paired test (for t-test, wilcoxon)");
DEFINE_double(mu0, 0.0, "Hypothesized population mean");
DEFINE_double(sigma, 0.0, "Known population standard deviation");
DEFINE_string(alternative, "two-sided", "Alternative hypothesis (two-sided, less, greater)");
/// @}

/// @name Global flags - Confidence intervals
/// @{
DEFINE_double(moe, 0.0, "Margin of error (for sample size calculation)");
DEFINE_int64(successes, -1, "Number of successes (for proportion CI)");
DEFINE_int64(trials, -1, "Number of trials (for proportion CI)");
/// @}

/// @name Global flags - Power analysis
/// @{
DEFINE_double(effect, 0.0, "Effect size (Cohen's d for t-test)");
DEFINE_int64(n, 0, "Sample size (for power analysis)");
DEFINE_double(power, 0.80, "Statistical power (1-beta)");
DEFINE_double(ratio, 1.0, "Sample size ratio n2/n1 (for two-sample)");
DEFINE_double(p1, 0.0, "Proportion 1 (for proportion test)");
DEFINE_double(p2, 0.0, "Proportion 2 (for proportion test)");
/// @}

/// @brief Usage message displayed with --help.
static const char* kUsageMessage =
    "statcpp - command-line statistics tool\n"
    "\n"
    "Usage:\n"
    "  statcpp <category> <command> [options] [file]\n"
    "  statcpp <shortcut> [options] [file]\n"
    "\n"
    "Categories:\n"
    "  desc       Descriptive statistics (mean, median, sd, summary, ...)\n"
    "  test       Statistical tests (t, welch, shapiro, mann-whitney, ...)\n"
    "  corr       Correlation & covariance (pearson, spearman, kendall, cov)\n"
    "  effect     Effect sizes (cohens-d, hedges-g, glass-delta, ...)\n"
    "  ci         Confidence intervals (mean, prop, var, diff, sample-size)\n"
    "  reg        Regression (simple, multiple, predict, residuals, vif)\n"
    "  anova      ANOVA (oneway, posthoc-tukey, posthoc-bonferroni, ...)\n"
    "  resample   Resampling (bootstrap-mean, bca, permtest, ...)\n"
    "  ts         Time series (acf, pacf, ma, ema, diff, mae, rmse, mape)\n"
    "  robust     Robust statistics (mad, outliers, winsorize, ...)\n"
    "  survival   Survival analysis (kaplan-meier, logrank, nelson-aalen)\n"
    "  cluster    Clustering (kmeans, hierarchical, silhouette)\n"
    "  multiple   Multiple testing correction (bonferroni, holm, bh)\n"
    "  glm        Generalized linear models (logistic, poisson)\n"
    "  model      Model selection (aic, cv, ridge, lasso)\n"
    "  power      Power analysis (t-one, t-two, prop)\n"
    "\n"
    "Shortcuts:\n"
    "  mean, median, mode, sd, var, summary, range, iqr, cv,\n"
    "  skewness, kurtosis, quartiles, gmean, hmean,\n"
    "  ttest, pearson, spearman, kendall\n"
    "\n"
    "Examples:\n"
    "  statcpp desc summary data.csv --col price\n"
    "  statcpp test t data.csv --col group1,group2\n"
    "  statcpp test t data.csv --col score --mu0 100\n"
    "  statcpp corr pearson data.csv --col height,weight\n"
    "  statcpp effect cohens-d data.csv --col before,after\n"
    "  statcpp ci mean data.csv --col price --level 0.99\n"
    "  statcpp ci sample-size --moe 0.03 --level 0.95\n";

/**
 * @brief Application entry point.
 *
 * Parses gflags, determines the command category and subcommand, optionally
 * reads CSV data from a file or stdin, and dispatches to the corresponding
 * command handler.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return 0 on success, 1 on error.
 */
int main(int argc, char* argv[]) {
    gflags::SetUsageMessage(kUsageMessage);
    gflags::SetVersionString(PROJECT_VERSION);
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    try {
        // Parse subcommand
        auto cmd = statcpp_cli::parse_command(argc, argv);

        // Some commands don't need CSV input
        bool needs_csv = !(cmd.category == "ci" && cmd.command == "sample-size")
                       && !(cmd.category == "ci" && cmd.command == "prop")
                       && !(cmd.category == "effect" && cmd.command == "cohens-h")
                       && !(cmd.category == "power");

        statcpp_cli::CsvData csv;
        if (needs_csv) {
            statcpp_cli::CsvReader reader;
            reader.has_header = FLAGS_header;
            if (!FLAGS_delimiter.empty()) {
                reader.delimiter = FLAGS_delimiter[0];
            }
            reader.set_na_string(FLAGS_na);

            if (FLAGS_row && cmd.filepath.empty()) {
                csv = reader.read_row_stdin();
            } else if (cmd.filepath.empty()) {
                csv = reader.read_stdin();
            } else {
                csv = reader.read_file(cmd.filepath);
            }
        }

        // Output mode
        statcpp_cli::OutputFormatter::Mode mode = statcpp_cli::OutputFormatter::Mode::Text;
        if (FLAGS_json) {
            mode = statcpp_cli::OutputFormatter::Mode::Json;
        } else if (FLAGS_quiet) {
            mode = statcpp_cli::OutputFormatter::Mode::Quiet;
        }
        statcpp_cli::OutputFormatter fmt(mode);

        // Dispatch
        if (cmd.category == "desc") {
            return statcpp_cli::commands::run_desc(cmd, csv, fmt);
        } else if (cmd.category == "test") {
            return statcpp_cli::commands::run_test(cmd, csv, fmt);
        } else if (cmd.category == "corr") {
            return statcpp_cli::commands::run_corr(cmd, csv, fmt);
        } else if (cmd.category == "ci") {
            return statcpp_cli::commands::run_ci(cmd, csv, fmt);
        } else if (cmd.category == "effect") {
            return statcpp_cli::commands::run_effect(cmd, csv, fmt);
        } else if (cmd.category == "reg") {
            return statcpp_cli::commands::run_reg(cmd, csv, fmt);
        } else if (cmd.category == "anova") {
            return statcpp_cli::commands::run_anova(cmd, csv, fmt);
        } else if (cmd.category == "resample") {
            return statcpp_cli::commands::run_resample(cmd, csv, fmt);
        } else if (cmd.category == "ts") {
            return statcpp_cli::commands::run_ts(cmd, csv, fmt);
        } else if (cmd.category == "robust") {
            return statcpp_cli::commands::run_robust(cmd, csv, fmt);
        } else if (cmd.category == "survival") {
            return statcpp_cli::commands::run_survival(cmd, csv, fmt);
        } else if (cmd.category == "cluster") {
            return statcpp_cli::commands::run_cluster(cmd, csv, fmt);
        } else if (cmd.category == "multiple") {
            return statcpp_cli::commands::run_multiple(cmd, csv, fmt);
        } else if (cmd.category == "power") {
            return statcpp_cli::commands::run_power(cmd, csv, fmt);
        } else if (cmd.category == "glm") {
            return statcpp_cli::commands::run_glm(cmd, csv, fmt);
        } else if (cmd.category == "model") {
            return statcpp_cli::commands::run_model(cmd, csv, fmt);
        } else {
            std::cerr << "Category '" << cmd.category << "' is not yet implemented." << std::endl;
            return 1;
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
