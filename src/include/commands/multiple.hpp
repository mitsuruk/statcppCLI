/**
 * @file multiple.hpp
 * @brief Command handler for multiple testing correction methods.
 *
 * Provides implementations of common p-value adjustment procedures for
 * controlling family-wise error rate (FWER) and false discovery rate (FDR)
 * when performing multiple hypothesis tests simultaneously:
 * - Bonferroni correction (FWER control)
 * - Holm-Bonferroni step-down procedure (FWER control)
 * - Benjamini-Hochberg procedure (FDR control)
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
DECLARE_bool(fail_na);
DECLARE_double(alpha);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Execute a multiple testing correction command.
 *
 * Reads a single column of raw p-values from the CSV data and applies the
 * correction method specified by @p cmd.command. Supported sub-commands:
 * - @c "bonferroni" -- Bonferroni correction: adjusts p-values by multiplying
 *   each by the total number of tests @e m, and reports the adjusted
 *   significance threshold (alpha / m).
 * - @c "holm" -- Holm-Bonferroni step-down procedure: ranks p-values in
 *   ascending order and applies progressively less conservative multipliers,
 *   providing a uniformly more powerful alternative to plain Bonferroni.
 * - @c "bh" -- Benjamini-Hochberg procedure: controls the false discovery
 *   rate (FDR) using a step-up algorithm, suitable when a controlled
 *   proportion of false positives is acceptable.
 *
 * The significance level is determined by the @c --alpha flag. The column of
 * p-values is specified by the @c --col flag (exactly one column required).
 * Results are printed in either human-readable text or structured format
 * depending on the output formatter mode.
 *
 * @param cmd  Parsed command whose @c command field selects the correction
 *             method ("bonferroni", "holm", or "bh").
 * @param csv  CSV data source providing the column of p-values.
 * @param fmt  Output formatter controlling the display mode (text or
 *             structured) and responsible for flushing final output.
 * @return 0 on success.
 * @throws std::runtime_error If the column specification does not contain
 *         exactly one column, or if @p cmd.command is not a recognised
 *         correction method.
 */
inline int run_multiple(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("multiple." + cmd.command);

    if (cmd.command == "bonferroni") {
        // Input: column of p-values
        if (cols.size() != 1) {
            throw std::runtime_error("bonferroni requires 1 column of p-values");
        }
        auto pvals = csv.get_clean_data(cols[0], FLAGS_fail_na);
        std::size_t m = pvals.size();
        double adjusted_alpha = FLAGS_alpha / static_cast<double>(m);

        fmt.set_input_info({{"column", cols[0]}, {"n_tests", m}, {"alpha", FLAGS_alpha}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Bonferroni Correction:" << std::endl;
            fmt.print("Adjusted alpha", adjusted_alpha);
            std::cout << "\n  " << std::left << std::setw(8) << "Test"
                      << std::setw(12) << "p-value"
                      << std::setw(14) << "Adj. p-value"
                      << "Sig?" << std::endl;
            for (std::size_t i = 0; i < m; ++i) {
                double adj_p = std::min(pvals[i] * static_cast<double>(m), 1.0);
                bool sig = adj_p < FLAGS_alpha;
                std::cout << "  " << std::left << std::setw(8) << (i + 1)
                          << std::setw(12) << pvals[i]
                          << std::setw(14) << adj_p
                          << (sig ? "Yes" : "No") << std::endl;
            }
        } else {
            fmt.print("Adjusted alpha", adjusted_alpha);
            for (std::size_t i = 0; i < m; ++i) {
                double adj_p = std::min(pvals[i] * static_cast<double>(m), 1.0);
                fmt.print("p" + std::to_string(i + 1) + " adjusted", adj_p);
            }
        }

    } else if (cmd.command == "holm") {
        if (cols.size() != 1) {
            throw std::runtime_error("holm requires 1 column of p-values");
        }
        auto pvals = csv.get_clean_data(cols[0], FLAGS_fail_na);
        std::size_t m = pvals.size();

        // Sort p-values, keeping track of original indices
        std::vector<std::size_t> idx(m);
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(),
                  [&](std::size_t a, std::size_t b) { return pvals[a] < pvals[b]; });

        // Compute adjusted p-values (Holm step-down)
        std::vector<double> adj_p(m);
        double running_max = 0.0;
        for (std::size_t i = 0; i < m; ++i) {
            double p_adj = pvals[idx[i]] * static_cast<double>(m - i);
            running_max = std::max(running_max, p_adj);
            adj_p[idx[i]] = std::min(running_max, 1.0);
        }

        fmt.set_input_info({{"column", cols[0]}, {"n_tests", m}, {"alpha", FLAGS_alpha}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Holm-Bonferroni Correction:" << std::endl;
            std::cout << "  " << std::left << std::setw(8) << "Test"
                      << std::setw(12) << "p-value"
                      << std::setw(14) << "Adj. p-value"
                      << "Sig?" << std::endl;
            for (std::size_t i = 0; i < m; ++i) {
                bool sig = adj_p[i] < FLAGS_alpha;
                std::cout << "  " << std::left << std::setw(8) << (i + 1)
                          << std::setw(12) << pvals[i]
                          << std::setw(14) << adj_p[i]
                          << (sig ? "Yes" : "No") << std::endl;
            }
        } else {
            for (std::size_t i = 0; i < m; ++i) {
                fmt.print("p" + std::to_string(i + 1) + " adjusted", adj_p[i]);
            }
        }

    } else if (cmd.command == "bh") {
        // Benjamini-Hochberg (FDR)
        if (cols.size() != 1) {
            throw std::runtime_error("bh requires 1 column of p-values");
        }
        auto pvals = csv.get_clean_data(cols[0], FLAGS_fail_na);
        std::size_t m = pvals.size();

        std::vector<std::size_t> idx(m);
        std::iota(idx.begin(), idx.end(), 0);
        std::sort(idx.begin(), idx.end(),
                  [&](std::size_t a, std::size_t b) { return pvals[a] < pvals[b]; });

        // Compute BH adjusted p-values (step-up)
        std::vector<double> adj_p(m);
        double running_min = 1.0;
        for (std::size_t i = m; i > 0; --i) {
            double p_adj = pvals[idx[i - 1]] * static_cast<double>(m) / static_cast<double>(i);
            running_min = std::min(running_min, p_adj);
            adj_p[idx[i - 1]] = std::min(running_min, 1.0);
        }

        fmt.set_input_info({{"column", cols[0]}, {"n_tests", m}, {"alpha", FLAGS_alpha},
                           {"method", "Benjamini-Hochberg"}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Benjamini-Hochberg (FDR) Correction:" << std::endl;
            std::cout << "  " << std::left << std::setw(8) << "Test"
                      << std::setw(12) << "p-value"
                      << std::setw(14) << "Adj. p-value"
                      << "Sig?" << std::endl;
            for (std::size_t i = 0; i < m; ++i) {
                bool sig = adj_p[i] < FLAGS_alpha;
                std::cout << "  " << std::left << std::setw(8) << (i + 1)
                          << std::setw(12) << pvals[i]
                          << std::setw(14) << adj_p[i]
                          << (sig ? "Yes" : "No") << std::endl;
            }
        } else {
            for (std::size_t i = 0; i < m; ++i) {
                fmt.print("p" + std::to_string(i + 1) + " adjusted", adj_p[i]);
            }
        }

    } else {
        throw std::runtime_error("Unknown multiple command: " + cmd.command +
            "\nAvailable: bonferroni, holm, bh");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
