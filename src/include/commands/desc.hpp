/**
 * @file desc.hpp
 * @brief Descriptive statistics command handler.
 *
 * Provides the command dispatch logic for all descriptive statistics
 * sub-commands (e.g. summary, mean, median, mode, variance, standard
 * deviation, range, IQR, CV, skewness, kurtosis, percentile, quartiles,
 * five-number summary, geometric mean, harmonic mean, and trimmed mean).
 */
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <statcpp/statcpp.hpp>

#include "cli_parser.hpp"
#include "csv_reader.hpp"
#include "output_formatter.hpp"

DECLARE_string(col);
DECLARE_bool(population);
DECLARE_bool(presorted);
DECLARE_bool(fail_na);
DECLARE_double(p);
DECLARE_double(trim);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Prepare a sorted copy of the data, or return it as-is if already sorted.
 *
 * When @p presorted is @c true the input vector is returned directly,
 * avoiding an unnecessary copy and sort. Otherwise a sorted copy is
 * created and returned.
 *
 * @param data      The numeric data to sort.
 * @param presorted If @c true, assume @p data is already sorted in
 *                  ascending order and skip sorting.
 * @return A vector containing the same elements as @p data in ascending order.
 */
inline std::vector<double> prepare_sorted(const std::vector<double>& data, bool presorted) {
    if (presorted) return data;
    auto sorted = data;
    std::sort(sorted.begin(), sorted.end());
    return sorted;
}

/**
 * @brief Execute a descriptive statistics sub-command.
 *
 * Reads the column specified by the @c --col flag from the CSV data,
 * dispatches to the appropriate statcpp function based on @p cmd.command,
 * and writes the result through the output formatter.
 *
 * Supported sub-commands: @c summary, @c mean, @c median, @c mode,
 * @c var, @c sd, @c range, @c iqr, @c cv, @c skewness, @c kurtosis,
 * @c percentile, @c quartiles, @c five-number, @c gmean, @c hmean,
 * and @c trimmed-mean.
 *
 * @param cmd  The parsed command containing the sub-command name.
 * @param csv  The CSV data source from which the target column is read.
 * @param fmt  The output formatter used to print results.
 * @return 0 on success.
 * @throws std::runtime_error If @c --col is not specified, if the column
 *         contains no valid data after removing missing values, or if
 *         an unknown sub-command is requested.
 */
inline int run_desc(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    if (FLAGS_col.empty()) {
        throw std::runtime_error("--col is required for desc commands");
    }

    auto data = csv.get_clean_data(FLAGS_col, FLAGS_fail_na);
    if (data.empty()) {
        throw std::runtime_error("No data after removing missing values");
    }

    std::size_t ddof = FLAGS_population ? 0 : 1;

    const auto& col_info = csv.get_column(FLAGS_col);
    fmt.set_command("desc." + cmd.command);
    fmt.set_input_info({
        {"column", col_info.name},
        {"n", data.size()}
    });

    if (cmd.command == "summary") {
        auto sorted = prepare_sorted(data, FLAGS_presorted);
        auto fns = statcpp::five_number_summary(sorted.begin(), sorted.end());
        double m = statcpp::mean(data.begin(), data.end());

        fmt.print({
            {"Count",    static_cast<double>(data.size())},
            {"Mean",     m},
            {"Std Dev",  statcpp::stdev(data.begin(), data.end(), m, ddof)},
            {"Min",      fns.min},
            {"Q1",       fns.q1},
            {"Median",   fns.median},
            {"Q3",       fns.q3},
            {"Max",      fns.max},
            {"Skewness", statcpp::skewness(data.begin(), data.end())},
            {"Kurtosis", statcpp::kurtosis(data.begin(), data.end())},
        });

    } else if (cmd.command == "mean") {
        double val = statcpp::mean(data.begin(), data.end());
        fmt.print("Mean", val);

    } else if (cmd.command == "median") {
        auto sorted = prepare_sorted(data, FLAGS_presorted);
        double val = statcpp::median(sorted.begin(), sorted.end());
        fmt.print("Median", val);

    } else if (cmd.command == "mode") {
        auto m = statcpp::modes(data.begin(), data.end());
        if (m.size() == 1) {
            fmt.print("Mode", m[0]);
        } else {
            for (std::size_t i = 0; i < m.size(); ++i) {
                fmt.print("Mode[" + std::to_string(i + 1) + "]", m[i]);
            }
        }

    } else if (cmd.command == "var") {
        double val = statcpp::var(data.begin(), data.end(), ddof);
        fmt.print("Variance", val);

    } else if (cmd.command == "sd") {
        double val = statcpp::stdev(data.begin(), data.end(), ddof);
        fmt.print("Std Dev", val);

    } else if (cmd.command == "range") {
        double val = statcpp::range(data.begin(), data.end());
        fmt.print("Range", val);

    } else if (cmd.command == "iqr") {
        auto sorted = prepare_sorted(data, FLAGS_presorted);
        double val = statcpp::iqr(sorted.begin(), sorted.end());
        fmt.print("IQR", val);

    } else if (cmd.command == "cv") {
        double val = statcpp::coefficient_of_variation(data.begin(), data.end());
        fmt.print("CV", val);

    } else if (cmd.command == "skewness") {
        double val = statcpp::skewness(data.begin(), data.end());
        fmt.print("Skewness", val);

    } else if (cmd.command == "kurtosis") {
        double val = statcpp::kurtosis(data.begin(), data.end());
        fmt.print("Kurtosis", val);

    } else if (cmd.command == "percentile") {
        auto sorted = prepare_sorted(data, FLAGS_presorted);
        double val = statcpp::percentile(sorted.begin(), sorted.end(), FLAGS_p);
        fmt.print("P" + std::to_string(static_cast<int>(FLAGS_p * 100)), val);

    } else if (cmd.command == "quartiles") {
        auto sorted = prepare_sorted(data, FLAGS_presorted);
        auto q = statcpp::quartiles(sorted.begin(), sorted.end());
        fmt.print({
            {"Q1", q.q1},
            {"Q2", q.q2},
            {"Q3", q.q3},
        });

    } else if (cmd.command == "five-number") {
        auto sorted = prepare_sorted(data, FLAGS_presorted);
        auto fns = statcpp::five_number_summary(sorted.begin(), sorted.end());
        fmt.print({
            {"Min",    fns.min},
            {"Q1",     fns.q1},
            {"Median", fns.median},
            {"Q3",     fns.q3},
            {"Max",    fns.max},
        });

    } else if (cmd.command == "gmean") {
        double val = statcpp::geometric_mean(data.begin(), data.end());
        fmt.print("Geometric Mean", val);

    } else if (cmd.command == "hmean") {
        double val = statcpp::harmonic_mean(data.begin(), data.end());
        fmt.print("Harmonic Mean", val);

    } else if (cmd.command == "trimmed-mean") {
        double val = statcpp::trimmed_mean(data.begin(), data.end(), FLAGS_trim);
        fmt.print("Trimmed Mean", val);

    } else {
        throw std::runtime_error("Unknown desc command: " + cmd.command);
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
