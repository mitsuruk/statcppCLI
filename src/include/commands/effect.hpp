/**
 * @file effect.hpp
 * @brief Effect size command handler for the statcpp CLI.
 *
 * Provides the command dispatcher for computing various effect size measures:
 * Cohen's d, Hedges' g, Glass's delta, Cohen's h, odds ratio, and risk ratio.
 * Each sub-command reads data from CSV columns and outputs the computed
 * statistic along with an optional qualitative interpretation.
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
DECLARE_double(mu0);
DECLARE_double(sigma);
DECLARE_double(p1);
DECLARE_double(p2);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Interpret an effect size value using Cohen's conventional thresholds.
 *
 * Maps the absolute value of the given effect size @p d to a qualitative
 * label (negligible, small, medium, or large) via
 * @c statcpp::interpret_cohens_d.  The same thresholds are applicable to
 * Cohen's d, Hedges' g, and similar standardised mean-difference measures.
 *
 * @param d  The effect size value to interpret (sign is ignored).
 * @return A human-readable string: "negligible", "small", "medium", "large",
 *         or "unknown" if the magnitude does not match any known category.
 */
inline std::string interpret_d(double d) {
    auto mag = statcpp::interpret_cohens_d(std::abs(d));
    switch (mag) {
        case statcpp::effect_size_magnitude::negligible: return "negligible";
        case statcpp::effect_size_magnitude::small:      return "small";
        case statcpp::effect_size_magnitude::medium:     return "medium";
        case statcpp::effect_size_magnitude::large:      return "large";
    }
    return "unknown";
}

/**
 * @brief Execute an effect-size sub-command.
 *
 * Dispatches to the appropriate computation based on @p cmd.command:
 *
 * | Sub-command      | Description                                              | Required columns |
 * |------------------|----------------------------------------------------------|------------------|
 * | cohens-d         | Cohen's d (one-sample or two-sample)                     | 1 or 2           |
 * | hedges-g         | Hedges' g with bias correction (one-sample or two-sample)| 1 or 2           |
 * | glass-delta      | Glass's delta (control vs. treatment)                    | 2                |
 * | cohens-h         | Cohen's h for comparing two proportions (uses --p1, --p2)| n/a              |
 * | odds-ratio       | Odds ratio from a 2x2 contingency table                  | 1 (4 values)     |
 * | risk-ratio       | Risk ratio from a 2x2 contingency table                  | 1 (4 values)     |
 *
 * Column names are obtained from the @c --col flag (comma-separated).
 * For one-sample tests the population mean is taken from @c --mu0 and an
 * optional known standard deviation from @c --sigma.
 *
 * @param cmd  Parsed command whose @c command field identifies the sub-command.
 * @param csv  CSV data source from which column values are read.
 * @param fmt  Output formatter used to emit the result in text or JSON mode.
 * @return 0 on success.
 * @throws std::runtime_error If the sub-command is unknown, the column count
 *         is invalid for the chosen measure, the data length is wrong (e.g.
 *         odds-ratio / risk-ratio not receiving exactly 4 values), or required
 *         flags (--p1, --p2) are missing.
 */
inline int run_effect(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("effect." + cmd.command);

    if (cmd.command == "cohens-d") {
        if (cols.size() == 1) {
            auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
            double d;
            if (FLAGS_sigma > 0) {
                d = statcpp::cohens_d(data.begin(), data.end(), FLAGS_mu0, FLAGS_sigma);
            } else {
                d = statcpp::cohens_d(data.begin(), data.end(), FLAGS_mu0);
            }
            fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"mu0", FLAGS_mu0}});
            fmt.print("Cohen's d", d);
            if (fmt.get_mode() == OutputFormatter::Mode::Text) {
                std::cout << "  Interpretation: " << interpret_d(d) << std::endl;
            }
        } else if (cols.size() == 2) {
            auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
            auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
            double d = statcpp::cohens_d_two_sample(d1.begin(), d1.end(), d2.begin(), d2.end());
            fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
            fmt.print("Cohen's d", d);
            if (fmt.get_mode() == OutputFormatter::Mode::Text) {
                std::cout << "  Interpretation: " << interpret_d(d) << std::endl;
            }
        } else {
            throw std::runtime_error("cohens-d requires 1 or 2 columns");
        }

    } else if (cmd.command == "hedges-g") {
        if (cols.size() == 1) {
            auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
            double g = statcpp::hedges_g(data.begin(), data.end(), FLAGS_mu0);
            fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"mu0", FLAGS_mu0}});
            fmt.print("Hedges' g", g);
            if (fmt.get_mode() == OutputFormatter::Mode::Text) {
                std::cout << "  Interpretation: " << interpret_d(g) << std::endl;
            }
        } else if (cols.size() == 2) {
            auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
            auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
            double g = statcpp::hedges_g_two_sample(d1.begin(), d1.end(), d2.begin(), d2.end());
            fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
            fmt.print("Hedges' g", g);
            if (fmt.get_mode() == OutputFormatter::Mode::Text) {
                std::cout << "  Interpretation: " << interpret_d(g) << std::endl;
            }
        } else {
            throw std::runtime_error("hedges-g requires 1 or 2 columns");
        }

    } else if (cmd.command == "glass-delta") {
        if (cols.size() != 2) {
            throw std::runtime_error("glass-delta requires 2 columns (--col control,treatment)");
        }
        auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
        double d = statcpp::glass_delta(d1.begin(), d1.end(), d2.begin(), d2.end());
        fmt.set_input_info({{"control", cols[0]}, {"treatment", cols[1]}});
        fmt.print("Glass's Delta", d);

    } else if (cmd.command == "cohens-h") {
        if (FLAGS_p1 <= 0 || FLAGS_p2 <= 0) {
            throw std::runtime_error("cohens-h requires --p1 and --p2 (proportions in (0,1))");
        }
        double h = statcpp::cohens_h(FLAGS_p1, FLAGS_p2);
        fmt.set_input_info({{"p1", FLAGS_p1}, {"p2", FLAGS_p2}});
        fmt.print("Cohen's h", h);

    } else if (cmd.command == "odds-ratio") {
        // Expects 4 values: a,b,c,d from 2x2 table
        if (cols.size() != 1) {
            throw std::runtime_error("odds-ratio: pass a column with 4 values (a,b,c,d of 2x2 table)");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        if (data.size() != 4) {
            throw std::runtime_error("odds-ratio requires exactly 4 values (a,b,c,d)");
        }
        double or_val = statcpp::odds_ratio(data[0], data[1], data[2], data[3]);
        fmt.set_input_info({{"a", data[0]}, {"b", data[1]}, {"c", data[2]}, {"d", data[3]}});
        fmt.print("Odds Ratio", or_val);

    } else if (cmd.command == "risk-ratio") {
        if (cols.size() != 1) {
            throw std::runtime_error("risk-ratio: pass a column with 4 values (a,b,c,d of 2x2 table)");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        if (data.size() != 4) {
            throw std::runtime_error("risk-ratio requires exactly 4 values (a,b,c,d)");
        }
        double rr = statcpp::risk_ratio(data[0], data[1], data[2], data[3]);
        fmt.set_input_info({{"a", data[0]}, {"b", data[1]}, {"c", data[2]}, {"d", data[3]}});
        fmt.print("Risk Ratio", rr);

    } else {
        throw std::runtime_error("Unknown effect command: " + cmd.command);
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
