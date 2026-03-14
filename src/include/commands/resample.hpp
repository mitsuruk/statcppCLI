/**
 * @file resample.hpp
 * @brief Resampling-based command handler for the statcpp CLI.
 *
 * Provides the dispatch logic for resampling commands including
 * bootstrap estimation of mean, median, and standard deviation,
 * bias-corrected and accelerated (BCa) bootstrap confidence intervals,
 * and permutation tests for two-sample difference in means and
 * correlation significance.
 */
#pragma once

#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <statcpp/statcpp.hpp>

#include "cli_parser.hpp"
#include "csv_reader.hpp"
#include "output_formatter.hpp"

DECLARE_string(col);
DECLARE_bool(fail_na);
DECLARE_double(level);
DECLARE_bool(paired);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Execute a resampling sub-command.
 *
 * Dispatches the parsed command to the appropriate resampling routine:
 *
 * | Sub-command          | Description                                         | Columns |
 * |----------------------|-----------------------------------------------------|---------|
 * | bootstrap-mean       | Bootstrap estimate of the mean (B = 1000)           | 1       |
 * | bootstrap-median     | Bootstrap estimate of the median (B = 1000)         | 1       |
 * | bootstrap-sd         | Bootstrap estimate of the standard deviation (B = 1000) | 1   |
 * | bca                  | BCa bootstrap CI for the mean (B = 1000)            | 1       |
 * | permtest             | Permutation test for two-sample difference in means (10 000 permutations); supports paired mode via @c --paired | 2 |
 * | permtest-corr        | Permutation test for correlation significance (10 000 permutations) | 2 |
 *
 * Column names are read from the @c --col flag (comma-separated).
 * The confidence level is controlled by the @c --level flag.
 *
 * @param cmd  Parsed command containing the sub-command name and any positional arguments.
 * @param csv  CSV data source from which columns are extracted.
 * @param fmt  Output formatter used to render results (text, JSON, etc.).
 * @return 0 on success.
 * @throws std::runtime_error If the wrong number of columns is specified for a
 *         sub-command, or if the sub-command name is not recognised.
 */
inline int run_resample(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("resample." + cmd.command);

    if (cmd.command == "bootstrap-mean") {
        if (cols.size() != 1) {
            throw std::runtime_error("bootstrap-mean requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::bootstrap_mean(data.begin(), data.end(), 1000, FLAGS_level);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"B", 1000}});
        fmt.print({
            {"Estimate",  r.estimate},
            {"SE",        r.standard_error},
            {"CI Lower",  r.ci_lower},
            {"CI Upper",  r.ci_upper},
            {"Bias",      r.bias},
        });

    } else if (cmd.command == "bootstrap-median") {
        if (cols.size() != 1) {
            throw std::runtime_error("bootstrap-median requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::bootstrap_median(data.begin(), data.end(), 1000, FLAGS_level);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"B", 1000}});
        fmt.print({
            {"Estimate",  r.estimate},
            {"SE",        r.standard_error},
            {"CI Lower",  r.ci_lower},
            {"CI Upper",  r.ci_upper},
            {"Bias",      r.bias},
        });

    } else if (cmd.command == "bootstrap-sd") {
        if (cols.size() != 1) {
            throw std::runtime_error("bootstrap-sd requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::bootstrap_stddev(data.begin(), data.end(), 1000, FLAGS_level);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"B", 1000}});
        fmt.print({
            {"Estimate",  r.estimate},
            {"SE",        r.standard_error},
            {"CI Lower",  r.ci_lower},
            {"CI Upper",  r.ci_upper},
            {"Bias",      r.bias},
        });

    } else if (cmd.command == "bca") {
        // BCa bootstrap for the mean
        if (cols.size() != 1) {
            throw std::runtime_error("bca requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);

        auto stat_func = [](auto first, auto last) {
            return statcpp::mean(first, last);
        };
        auto r = statcpp::bootstrap_bca(data.begin(), data.end(), stat_func, 1000, FLAGS_level);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"B", 1000}, {"method", "BCa"}});
        fmt.print({
            {"Estimate",  r.estimate},
            {"SE",        r.standard_error},
            {"CI Lower",  r.ci_lower},
            {"CI Upper",  r.ci_upper},
            {"Bias",      r.bias},
        });

    } else if (cmd.command == "permtest") {
        // Permutation test for two-sample difference in means
        if (cols.size() != 2) {
            throw std::runtime_error("permtest requires 2 columns (--col col1,col2)");
        }

        if (FLAGS_paired) {
            auto [d1, d2] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
            auto r = statcpp::permutation_test_paired(
                d1.begin(), d1.end(), d2.begin(), d2.end(), 10000);

            fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n", d1.size()},
                              {"permutations", r.n_permutations}, {"paired", true}});
            fmt.print({
                {"Statistic", r.observed_statistic},
                {"p-value",   r.p_value},
            });
        } else {
            auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
            auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
            auto r = statcpp::permutation_test_two_sample(
                d1.begin(), d1.end(), d2.begin(), d2.end(), 10000);

            fmt.set_input_info({{"columns", {cols[0], cols[1]}},
                              {"n1", d1.size()}, {"n2", d2.size()},
                              {"permutations", r.n_permutations}});
            fmt.print({
                {"Statistic", r.observed_statistic},
                {"p-value",   r.p_value},
            });
        }

    } else if (cmd.command == "permtest-corr") {
        if (cols.size() != 2) {
            throw std::runtime_error("permtest-corr requires 2 columns (--col x,y)");
        }
        auto [d1, d2] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        auto r = statcpp::permutation_test_correlation(
            d1.begin(), d1.end(), d2.begin(), d2.end(), 10000);

        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n", d1.size()},
                          {"permutations", r.n_permutations}});
        fmt.print({
            {"Correlation", r.observed_statistic},
            {"p-value",     r.p_value},
        });

    } else {
        throw std::runtime_error("Unknown resample command: " + cmd.command +
            "\nAvailable: bootstrap-mean, bootstrap-median, bootstrap-sd, bca, "
            "permtest, permtest-corr");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
