/**
 * @file ci.hpp
 * @brief Confidence interval command handler.
 *
 * Provides the CLI entry point for computing confidence intervals
 * for a population mean, difference of two means, a proportion (Wilson
 * method), a variance, and for determining the required sample size
 * given a desired margin of error.
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
DECLARE_double(sigma);
DECLARE_double(moe);
DECLARE_int64(successes);
DECLARE_int64(trials);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Print a confidence interval result through the output formatter.
 *
 * Emits four key-value pairs: the point estimate, the lower and upper
 * bounds, and the confidence level.
 *
 * @param fmt Output formatter used to render the results.
 * @param ci  The confidence interval to print.
 */
inline void print_ci(OutputFormatter& fmt, const statcpp::confidence_interval& ci) {
    fmt.print({
        {"Estimate", ci.point_estimate},
        {"Lower",    ci.lower},
        {"Upper",    ci.upper},
        {"Level",    ci.confidence_level},
    });
}

/**
 * @brief Execute a confidence interval sub-command.
 *
 * Dispatches to the appropriate statcpp routine based on the sub-command
 * stored in @p cmd.command:
 *
 * | Sub-command     | Description                                                |
 * |-----------------|------------------------------------------------------------|
 * | `mean`          | CI for a single mean (z or t depending on `--sigma`).      |
 * | `diff`          | CI for the difference of two independent means.            |
 * | `prop`          | Wilson CI for a proportion (`--successes`, `--trials`).     |
 * | `var`           | CI for a population variance (chi-squared method).         |
 * | `sample-size`   | Required sample size for a given margin of error (`--moe`).|
 *
 * @param cmd Parsed command containing the sub-command name.
 * @param csv CSV data source from which column data is read.
 * @param fmt Output formatter used to render the results.
 * @return 0 on success.
 * @throws std::runtime_error If the sub-command is unknown, required flags
 *         are missing or invalid, or the data set is empty.
 */
inline int run_ci(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    fmt.set_command("ci." + cmd.command);

    if (cmd.command == "mean") {
        auto cols = CsvData::split_col_spec(FLAGS_col);
        if (cols.size() == 1) {
            auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
            if (data.empty()) throw std::runtime_error("No data");

            statcpp::confidence_interval ci;
            if (FLAGS_sigma > 0) {
                ci = statcpp::ci_mean_z(data.begin(), data.end(), FLAGS_sigma, FLAGS_level);
            } else {
                ci = statcpp::ci_mean(data.begin(), data.end(), FLAGS_level);
            }
            fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
            print_ci(fmt, ci);
        } else if (cols.size() == 2) {
            throw std::runtime_error("For difference of means, use 'ci diff'");
        } else {
            throw std::runtime_error("ci mean requires 1 column (--col col)");
        }

    } else if (cmd.command == "diff") {
        auto cols = CsvData::split_col_spec(FLAGS_col);
        if (cols.size() != 2) {
            throw std::runtime_error("ci diff requires 2 columns (--col col1,col2)");
        }
        auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
        auto ci = statcpp::ci_mean_diff(d1.begin(), d1.end(), d2.begin(), d2.end(), FLAGS_level);
        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
        print_ci(fmt, ci);

    } else if (cmd.command == "prop") {
        if (FLAGS_successes < 0 || FLAGS_trials <= 0) {
            throw std::runtime_error("ci prop requires --successes and --trials");
        }
        auto ci = statcpp::ci_proportion_wilson(
            static_cast<std::size_t>(FLAGS_successes),
            static_cast<std::size_t>(FLAGS_trials),
            FLAGS_level);
        fmt.set_input_info({{"successes", FLAGS_successes}, {"trials", FLAGS_trials}});
        print_ci(fmt, ci);

    } else if (cmd.command == "var") {
        auto cols = CsvData::split_col_spec(FLAGS_col);
        if (cols.size() != 1) {
            throw std::runtime_error("ci var requires 1 column (--col col)");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto ci = statcpp::ci_variance(data.begin(), data.end(), FLAGS_level);
        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
        print_ci(fmt, ci);

    } else if (cmd.command == "sample-size") {
        if (FLAGS_moe <= 0) {
            throw std::runtime_error("sample-size requires --moe (margin of error)");
        }
        std::size_t n;
        if (FLAGS_sigma > 0) {
            n = statcpp::sample_size_for_moe_mean(FLAGS_moe, FLAGS_sigma, FLAGS_level);
            fmt.set_input_info({{"moe", FLAGS_moe}, {"sigma", FLAGS_sigma}, {"level", FLAGS_level}});
        } else {
            n = statcpp::sample_size_for_moe_proportion(FLAGS_moe, FLAGS_level);
            fmt.set_input_info({{"moe", FLAGS_moe}, {"level", FLAGS_level}});
        }
        fmt.print("Sample Size", static_cast<double>(n));

    } else {
        throw std::runtime_error("Unknown ci command: " + cmd.command);
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
