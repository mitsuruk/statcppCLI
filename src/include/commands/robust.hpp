/**
 * @file robust.hpp
 * @brief Command handler for robust statistics.
 *
 * Provides the entry point for robust statistical analysis commands including
 * Median Absolute Deviation (MAD), outlier detection via IQR / Z-score /
 * modified Z-score methods, winsorization, Hodges-Lehmann estimator, and
 * biweight midvariance.
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
DECLARE_double(trim);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Execute a robust statistics sub-command.
 *
 * Dispatches to the appropriate robust statistic routine based on
 * @p cmd.command.  Supported sub-commands:
 *
 * | Sub-command          | Description                                    |
 * |----------------------|------------------------------------------------|
 * | mad                  | Median Absolute Deviation (raw and scaled)     |
 * | outliers             | Outlier detection using the IQR method         |
 * | outliers-zscore      | Outlier detection using Z-score                |
 * | outliers-modified    | Outlier detection using modified Z-score       |
 * | winsorize            | Winsorized mean/SD compared with originals     |
 * | hodges-lehmann       | Hodges-Lehmann location estimator              |
 * | biweight             | Biweight midvariance                           |
 *
 * All sub-commands operate on a single column specified by the @c --col flag.
 *
 * @param cmd  Parsed command containing the sub-command name and any
 *             additional arguments.
 * @param csv  CSV data source from which the target column is read.
 * @param fmt  Output formatter used to render results (text or structured).
 * @return 0 on success.
 * @throws std::runtime_error If the sub-command requires exactly 1 column and
 *         a different number is provided, or if an unknown sub-command is
 *         specified.
 */
inline int run_robust(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("robust." + cmd.command);

    if (cmd.command == "mad") {
        if (cols.size() != 1) {
            throw std::runtime_error("mad requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        double mad_val = statcpp::mad(data.begin(), data.end());
        double mad_s = statcpp::mad_scaled(data.begin(), data.end());

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
        fmt.print({
            {"MAD",        mad_val},
            {"MAD scaled", mad_s},
        });

    } else if (cmd.command == "outliers") {
        if (cols.size() != 1) {
            throw std::runtime_error("outliers requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::detect_outliers_iqr(data.begin(), data.end());

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"method", "IQR"}});
        fmt.print({
            {"Q1",           r.q1},
            {"Q3",           r.q3},
            {"IQR",          r.iqr_value},
            {"Lower fence",  r.lower_fence},
            {"Upper fence",  r.upper_fence},
            {"N outliers",   static_cast<double>(r.outliers.size())},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text && !r.outliers.empty()) {
            std::cout << "  Outliers:";
            for (std::size_t i = 0; i < r.outliers.size(); ++i) {
                std::cout << " " << r.outliers[i];
                if (i < r.outliers.size() - 1) std::cout << ",";
            }
            std::cout << std::endl;
        }

    } else if (cmd.command == "outliers-zscore") {
        if (cols.size() != 1) {
            throw std::runtime_error("outliers-zscore requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::detect_outliers_zscore(data.begin(), data.end());

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"method", "Z-score"}});
        fmt.print({
            {"Lower fence",  r.lower_fence},
            {"Upper fence",  r.upper_fence},
            {"N outliers",   static_cast<double>(r.outliers.size())},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text && !r.outliers.empty()) {
            std::cout << "  Outliers:";
            for (std::size_t i = 0; i < r.outliers.size(); ++i) {
                std::cout << " " << r.outliers[i];
                if (i < r.outliers.size() - 1) std::cout << ",";
            }
            std::cout << std::endl;
        }

    } else if (cmd.command == "outliers-modified") {
        if (cols.size() != 1) {
            throw std::runtime_error("outliers-modified requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::detect_outliers_modified_zscore(data.begin(), data.end());

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"method", "Modified Z-score"}});
        fmt.print({
            {"Lower fence",  r.lower_fence},
            {"Upper fence",  r.upper_fence},
            {"N outliers",   static_cast<double>(r.outliers.size())},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text && !r.outliers.empty()) {
            std::cout << "  Outliers:";
            for (std::size_t i = 0; i < r.outliers.size(); ++i) {
                std::cout << " " << r.outliers[i];
                if (i < r.outliers.size() - 1) std::cout << ",";
            }
            std::cout << std::endl;
        }

    } else if (cmd.command == "winsorize") {
        if (cols.size() != 1) {
            throw std::runtime_error("winsorize requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto winsorized = statcpp::winsorize(data.begin(), data.end(), FLAGS_trim);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"limits", FLAGS_trim}});

        // Show summary of winsorized data
        double wmean = statcpp::mean(winsorized.begin(), winsorized.end());
        double wsd = statcpp::stdev(winsorized.begin(), winsorized.end(), 1);
        fmt.print({
            {"Mean (original)",    statcpp::mean(data.begin(), data.end())},
            {"Mean (winsorized)",  wmean},
            {"SD (original)",      statcpp::stdev(data.begin(), data.end(), 1)},
            {"SD (winsorized)",    wsd},
        });

    } else if (cmd.command == "hodges-lehmann") {
        if (cols.size() != 1) {
            throw std::runtime_error("hodges-lehmann requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        double hl = statcpp::hodges_lehmann(data.begin(), data.end());

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
        fmt.print("Hodges-Lehmann", hl);

    } else if (cmd.command == "biweight") {
        if (cols.size() != 1) {
            throw std::runtime_error("biweight requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        double bw = statcpp::biweight_midvariance(data.begin(), data.end());

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
        fmt.print("Biweight midvariance", bw);

    } else {
        throw std::runtime_error("Unknown robust command: " + cmd.command +
            "\nAvailable: mad, outliers, outliers-zscore, outliers-modified, "
            "winsorize, hodges-lehmann, biweight");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
