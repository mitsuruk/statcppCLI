/**
 * @file corr.hpp
 * @brief Correlation and covariance command handler.
 *
 * Provides the implementation for the "corr" family of subcommands:
 * - **pearson**  -- Pearson product-moment correlation coefficient
 * - **spearman** -- Spearman rank correlation coefficient
 * - **kendall**  -- Kendall tau rank correlation coefficient
 * - **cov**      -- Sample or population covariance
 * - **matrix**   -- Pearson correlation matrix for multiple columns
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
DECLARE_bool(population);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Execute a correlation / covariance subcommand.
 *
 * Dispatches to the appropriate statcpp routine based on
 * @p cmd.command and writes the result through the output formatter.
 *
 * | Subcommand | Required columns | Description                        |
 * |------------|------------------|------------------------------------|
 * | pearson    | 2                | Pearson correlation coefficient    |
 * | spearman   | 2                | Spearman rank correlation          |
 * | kendall    | 2                | Kendall tau rank correlation       |
 * | cov        | 2                | Sample/population covariance       |
 * | matrix     | 2+               | Pearson correlation matrix         |
 *
 * Column selection is controlled by the @c --col flag
 * (comma-separated column names).  Missing values are removed
 * pairwise; @c --fail_na causes an error if any NA is encountered.
 * For the @c cov subcommand, @c --population switches between
 * population and sample covariance.
 *
 * @param cmd  Parsed command containing the subcommand name
 *             (e.g. "pearson", "spearman", "kendall", "cov", "matrix").
 * @param csv  CSV data source from which column data is retrieved.
 * @param fmt  Output formatter used to emit results in text or JSON mode.
 * @return 0 on success.
 * @throws std::runtime_error If the subcommand is unknown, the wrong
 *         number of columns is specified, or no data remains after
 *         removing missing values.
 */
inline int run_corr(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("corr." + cmd.command);

    if (cmd.command == "pearson" || cmd.command == "spearman" || cmd.command == "kendall") {
        if (cols.size() != 2) {
            throw std::runtime_error(cmd.command + " requires 2 columns (--col col1,col2)");
        }
        auto [d1, d2] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        if (d1.empty()) {
            throw std::runtime_error("No data after removing missing values");
        }

        double r;
        if (cmd.command == "pearson") {
            r = statcpp::pearson_correlation(d1.begin(), d1.end(), d2.begin(), d2.end());
        } else if (cmd.command == "spearman") {
            r = statcpp::spearman_correlation(d1.begin(), d1.end(), d2.begin(), d2.end());
        } else {
            r = statcpp::kendall_tau(d1.begin(), d1.end(), d2.begin(), d2.end());
        }

        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n", d1.size()}});
        fmt.print("r", r);

    } else if (cmd.command == "cov") {
        if (cols.size() != 2) {
            throw std::runtime_error("cov requires 2 columns (--col col1,col2)");
        }
        auto [d1, d2] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        if (d1.empty()) {
            throw std::runtime_error("No data after removing missing values");
        }

        double c;
        if (FLAGS_population) {
            c = statcpp::population_covariance(d1.begin(), d1.end(), d2.begin(), d2.end());
        } else {
            c = statcpp::sample_covariance(d1.begin(), d1.end(), d2.begin(), d2.end());
        }

        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n", d1.size()}});
        fmt.print("Covariance", c);

    } else if (cmd.command == "matrix") {
        if (cols.size() < 2) {
            throw std::runtime_error("matrix requires 2+ columns (--col c1,c2,c3)");
        }

        // Collect clean data for each column
        std::vector<std::vector<double>> data;
        std::vector<std::string> names;
        for (const auto& c : cols) {
            data.push_back(csv.get_clean_data(c, FLAGS_fail_na));
            names.push_back(csv.get_column(c).name);
        }

        fmt.set_input_info({{"columns", names}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            // Print header
            std::cout << std::setw(14) << "";
            for (const auto& n : names) {
                std::cout << std::setw(12) << n;
            }
            std::cout << std::endl;

            // Print matrix
            for (std::size_t i = 0; i < names.size(); ++i) {
                std::cout << "  " << std::left << std::setw(12) << names[i];
                for (std::size_t j = 0; j < names.size(); ++j) {
                    double r = statcpp::pearson_correlation(
                        data[i].begin(), data[i].end(),
                        data[j].begin(), data[j].end());
                    std::cout << std::right << std::setw(12) << std::fixed
                              << std::setprecision(4) << r;
                }
                std::cout << std::endl;
            }
            std::cout << std::defaultfloat;
        } else {
            // JSON mode
            nlohmann::json matrix;
            for (std::size_t i = 0; i < names.size(); ++i) {
                nlohmann::json row;
                for (std::size_t j = 0; j < names.size(); ++j) {
                    double r = statcpp::pearson_correlation(
                        data[i].begin(), data[i].end(),
                        data[j].begin(), data[j].end());
                    row[names[j]] = r;
                }
                matrix[names[i]] = row;
            }
            fmt.print_json("corr.matrix",
                          {{"columns", names}},
                          {{"matrix", matrix}});
            return 0;
        }

    } else {
        throw std::runtime_error("Unknown corr command: " + cmd.command);
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
