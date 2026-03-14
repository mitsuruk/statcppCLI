/**
 * @file anova.hpp
 * @brief ANOVA command handler for the statcpp CLI.
 *
 * Provides the entry point for all ANOVA-related sub-commands:
 * - One-way ANOVA (F-test with between/within decomposition)
 * - Tukey HSD post-hoc pairwise comparisons
 * - Bonferroni-corrected post-hoc pairwise comparisons
 * - Scheffe post-hoc pairwise comparisons
 * - Effect-size measures (eta-squared, omega-squared, Cohen's f)
 *
 * Results are emitted through an OutputFormatter so they can be rendered
 * as human-readable text, JSON, or quiet (value-only) output.
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
DECLARE_double(alpha);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Print a single row of the ANOVA table to standard output.
 *
 * Formats and prints one source row (e.g. "Between" or "Within") of the
 * ANOVA summary table.  Output is produced only when the formatter is in
 * text mode; in JSON or quiet mode this function is a no-op.
 *
 * @param fmt    The output formatter that controls the current output mode.
 * @param source Label for the variance source (e.g. "Between", "Within").
 * @param row    An @c anova_row containing SS, df, MS, F-statistic, and
 *               p-value for this source.
 */
inline void print_anova_row(OutputFormatter& fmt, const std::string& source,
                             const statcpp::anova_row& row) {
    if (fmt.get_mode() == OutputFormatter::Mode::Text) {
        std::cout << "  " << std::left << std::setw(14) << source
                  << std::setw(12) << row.ss
                  << std::setw(8) << row.df
                  << std::setw(12) << row.ms
                  << std::setw(12) << row.f_statistic
                  << row.p_value << std::endl;
    }
}

/**
 * @brief Execute an ANOVA sub-command and print results.
 *
 * Dispatches to one of the supported ANOVA sub-commands based on
 * @c cmd.command:
 * - @c "oneway" -- one-way ANOVA with F-test, eta-squared, and
 *   omega-squared.
 * - @c "posthoc-tukey" -- Tukey HSD pairwise comparisons with
 *   confidence intervals.
 * - @c "posthoc-bonferroni" -- Bonferroni-corrected pairwise
 *   comparisons.
 * - @c "posthoc-scheffe" -- Scheffe pairwise comparisons.
 * - @c "eta-squared" -- effect-size measures (eta-squared,
 *   omega-squared, Cohen's f).
 *
 * Column names are taken from the @c --col flag (comma-separated).
 * Each column must contain at least one valid numeric value.  The
 * significance level is controlled by the @c --alpha flag.
 *
 * @param cmd  Parsed command containing the sub-command name and any
 *             positional arguments.
 * @param csv  CSV data source from which group columns are read.
 * @param fmt  Output formatter used to emit results in the selected
 *             output mode (text, JSON, or quiet).
 *
 * @return 0 on success.
 *
 * @throws std::runtime_error If fewer than 2 columns are specified
 *         for any sub-command, or if an unknown sub-command name is
 *         given.
 */
inline int run_anova(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("anova." + cmd.command);

    if (cmd.command == "oneway") {
        if (cols.size() < 2) {
            throw std::runtime_error("one-way ANOVA requires 2+ columns (--col g1,g2,g3)");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }

        auto r = statcpp::one_way_anova(groups);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"k", r.n_groups}, {"n", r.n_total}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  ANOVA Table:" << std::endl;
            std::cout << "  " << std::left << std::setw(14) << "Source"
                      << std::setw(12) << "SS"
                      << std::setw(8) << "df"
                      << std::setw(12) << "MS"
                      << std::setw(12) << "F"
                      << "p-value" << std::endl;
            print_anova_row(fmt, "Between", r.between);
            print_anova_row(fmt, "Within", r.within);
            std::cout << "  " << std::left << std::setw(14) << "Total"
                      << std::setw(12) << r.ss_total
                      << r.df_total << std::endl;
            std::cout << std::endl;
        }

        fmt.print({
            {"F-statistic", r.between.f_statistic},
            {"p-value",     r.between.p_value},
            {"eta-squared", statcpp::eta_squared(r)},
            {"omega-squared", statcpp::omega_squared(r)},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            bool reject = r.between.p_value < FLAGS_alpha;
            std::cout << "  " << (reject ? "Reject H0" : "Fail to reject H0")
                      << " (alpha=" << FLAGS_alpha << ")" << std::endl;
        }

    } else if (cmd.command == "posthoc-tukey") {
        if (cols.size() < 2) {
            throw std::runtime_error("posthoc-tukey requires 2+ columns (--col g1,g2,g3)");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }

        auto anova_r = statcpp::one_way_anova(groups);
        auto ph = statcpp::tukey_hsd(anova_r, groups, FLAGS_alpha);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"method", ph.method}, {"alpha", FLAGS_alpha}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Tukey HSD Post-hoc Comparisons:" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << "Comparison"
                      << std::setw(12) << "Diff"
                      << std::setw(12) << "p-value"
                      << std::setw(12) << "Lower"
                      << std::setw(12) << "Upper"
                      << "Sig?" << std::endl;
            for (const auto& c : ph.comparisons) {
                std::string label = cols[c.group1] + " vs " + cols[c.group2];
                std::cout << "  " << std::left << std::setw(16) << label
                          << std::setw(12) << c.mean_diff
                          << std::setw(12) << c.p_value
                          << std::setw(12) << c.lower
                          << std::setw(12) << c.upper
                          << (c.significant ? "Yes" : "No") << std::endl;
            }
        } else {
            // JSON / Quiet
            for (const auto& c : ph.comparisons) {
                std::string label = cols[c.group1] + " vs " + cols[c.group2];
                fmt.print(label + " diff", c.mean_diff);
                fmt.print(label + " p", c.p_value);
            }
        }

    } else if (cmd.command == "posthoc-bonferroni") {
        if (cols.size() < 2) {
            throw std::runtime_error("posthoc-bonferroni requires 2+ columns");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }

        auto anova_r = statcpp::one_way_anova(groups);
        auto ph = statcpp::bonferroni_posthoc(anova_r, FLAGS_alpha);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"method", ph.method}, {"alpha", FLAGS_alpha}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Bonferroni Post-hoc Comparisons:" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << "Comparison"
                      << std::setw(12) << "Diff"
                      << std::setw(12) << "p-value"
                      << "Sig?" << std::endl;
            for (const auto& c : ph.comparisons) {
                std::string label = cols[c.group1] + " vs " + cols[c.group2];
                std::cout << "  " << std::left << std::setw(16) << label
                          << std::setw(12) << c.mean_diff
                          << std::setw(12) << c.p_value
                          << (c.significant ? "Yes" : "No") << std::endl;
            }
        } else {
            for (const auto& c : ph.comparisons) {
                std::string label = cols[c.group1] + " vs " + cols[c.group2];
                fmt.print(label + " diff", c.mean_diff);
                fmt.print(label + " p", c.p_value);
            }
        }

    } else if (cmd.command == "posthoc-scheffe") {
        if (cols.size() < 2) {
            throw std::runtime_error("posthoc-scheffe requires 2+ columns");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }

        auto anova_r = statcpp::one_way_anova(groups);
        auto ph = statcpp::scheffe_posthoc(anova_r, FLAGS_alpha);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"method", ph.method}, {"alpha", FLAGS_alpha}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Scheffe Post-hoc Comparisons:" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << "Comparison"
                      << std::setw(12) << "Diff"
                      << std::setw(12) << "p-value"
                      << "Sig?" << std::endl;
            for (const auto& c : ph.comparisons) {
                std::string label = cols[c.group1] + " vs " + cols[c.group2];
                std::cout << "  " << std::left << std::setw(16) << label
                          << std::setw(12) << c.mean_diff
                          << std::setw(12) << c.p_value
                          << (c.significant ? "Yes" : "No") << std::endl;
            }
        } else {
            for (const auto& c : ph.comparisons) {
                std::string label = cols[c.group1] + " vs " + cols[c.group2];
                fmt.print(label + " diff", c.mean_diff);
                fmt.print(label + " p", c.p_value);
            }
        }

    } else if (cmd.command == "eta-squared") {
        if (cols.size() < 2) {
            throw std::runtime_error("eta-squared requires 2+ columns");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }
        auto r = statcpp::one_way_anova(groups);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"k", r.n_groups}});
        fmt.print({
            {"eta-squared",   statcpp::eta_squared(r)},
            {"omega-squared", statcpp::omega_squared(r)},
            {"Cohen's f",     statcpp::cohens_f(r)},
        });

    } else {
        throw std::runtime_error("Unknown anova command: " + cmd.command +
            "\nAvailable: oneway, posthoc-tukey, posthoc-bonferroni, posthoc-scheffe, eta-squared");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
