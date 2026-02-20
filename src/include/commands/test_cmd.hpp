/**
 * @file test_cmd.hpp
 * @brief Statistical hypothesis tests command handler.
 *
 * Provides the CLI entry point for running a variety of statistical tests:
 * t-test (one-sample, two-sample, paired), Welch's t-test, z-test, F-test,
 * Shapiro-Wilk normality test, Kolmogorov-Smirnov test, Mann-Whitney U test,
 * Wilcoxon signed-rank test, Kruskal-Wallis test, Levene's test,
 * Bartlett's test, and Chi-square goodness-of-fit test.
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
DECLARE_bool(paired);
DECLARE_double(mu0);
DECLARE_double(sigma);
DECLARE_string(alternative);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Parse a string representation of the alternative hypothesis.
 *
 * Converts a human-readable alternative hypothesis string into the
 * corresponding @c statcpp::alternative_hypothesis enum value.
 *
 * @param alt The alternative hypothesis string. Accepted values are
 *            @c "two-sided" (or @c "two.sided"), @c "less", and @c "greater".
 * @return The matching @c statcpp::alternative_hypothesis enumerator.
 * @throws std::runtime_error If @p alt does not match any known alternative.
 */
inline statcpp::alternative_hypothesis parse_alternative(const std::string& alt) {
    if (alt == "two-sided" || alt == "two.sided") return statcpp::alternative_hypothesis::two_sided;
    if (alt == "less") return statcpp::alternative_hypothesis::less;
    if (alt == "greater") return statcpp::alternative_hypothesis::greater;
    throw std::runtime_error("Unknown alternative: " + alt +
        " (use 'two-sided', 'less', or 'greater')");
}

/**
 * @brief Format and print a statistical test result.
 *
 * Outputs the test statistic, degrees of freedom, p-value, and significance
 * level through the given @c OutputFormatter. In text mode an additional
 * line indicating whether the null hypothesis is rejected is printed.
 *
 * @param fmt   The output formatter used to render the result.
 * @param r     The test result containing the statistic, degrees of freedom,
 *              and p-value.
 * @param alpha The significance level used to determine rejection of H0.
 */
inline void print_test_result(OutputFormatter& fmt, const statcpp::test_result& r,
                               double alpha) {
    fmt.print({
        {"Statistic", r.statistic},
        {"df",        r.df},
        {"p-value",   r.p_value},
        {"alpha",     alpha},
    });

    if (fmt.get_mode() == OutputFormatter::Mode::Text) {
        bool reject = r.p_value < alpha;
        std::cout << "  " << (reject ? "Reject H0" : "Fail to reject H0") << std::endl;
    }
}

/**
 * @brief Execute the specified statistical test command.
 *
 * Dispatches to the appropriate statistical test based on @c cmd.command and
 * the columns supplied via the @c --col flag. Supported sub-commands are:
 *
 * | Sub-command      | Test                                         |
 * |------------------|----------------------------------------------|
 * | @c t             | Student's t-test (one-sample / two-sample / paired) |
 * | @c welch         | Welch's t-test (two-sample, unequal variances)      |
 * | @c z             | z-test (known population standard deviation)         |
 * | @c f             | F-test for equality of two variances                 |
 * | @c shapiro       | Shapiro-Wilk normality test                          |
 * | @c ks            | Kolmogorov-Smirnov test against the normal distribution |
 * | @c mann-whitney  | Mann-Whitney U test                                  |
 * | @c wilcoxon      | Wilcoxon signed-rank test (one-sample / paired)      |
 * | @c kruskal       | Kruskal-Wallis H test                                |
 * | @c levene        | Levene's test for equality of variances              |
 * | @c bartlett      | Bartlett's test for equality of variances             |
 * | @c chisq         | Chi-square goodness-of-fit test                       |
 *
 * @param cmd The parsed CLI command whose @c command field selects the test.
 * @param csv The CSV data source from which column data is extracted.
 * @param fmt The output formatter for rendering results.
 * @return 0 on successful completion.
 * @throws std::runtime_error If the column specification is invalid for the
 *         chosen test, required flags are missing, or the sub-command is
 *         unknown.
 */
inline int run_test(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);
    auto alt = parse_alternative(FLAGS_alternative);

    fmt.set_command("test." + cmd.command);

    if (cmd.command == "t") {
        if (cols.size() == 1) {
            // One-sample t-test
            auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
            auto r = statcpp::t_test(data.begin(), data.end(), FLAGS_mu0, alt);
            fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"mu0", FLAGS_mu0}});
            print_test_result(fmt, r, FLAGS_alpha);
        } else if (cols.size() == 2) {
            if (FLAGS_paired) {
                auto [d1, d2] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
                auto r = statcpp::t_test_paired(d1.begin(), d1.end(), d2.begin(), d2.end(), alt);
                fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n", d1.size()}, {"paired", true}});
                print_test_result(fmt, r, FLAGS_alpha);
            } else {
                auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
                auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
                auto r = statcpp::t_test_two_sample(d1.begin(), d1.end(), d2.begin(), d2.end(), alt);
                fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
                print_test_result(fmt, r, FLAGS_alpha);
            }
        } else {
            throw std::runtime_error("t-test requires 1 or 2 columns (--col col1 or --col col1,col2)");
        }

    } else if (cmd.command == "welch") {
        if (cols.size() != 2) {
            throw std::runtime_error("Welch t-test requires 2 columns (--col col1,col2)");
        }
        auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
        auto r = statcpp::t_test_welch(d1.begin(), d1.end(), d2.begin(), d2.end(), alt);
        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "z") {
        if (cols.size() != 1) {
            throw std::runtime_error("z-test requires 1 column and --sigma");
        }
        if (FLAGS_sigma <= 0) {
            throw std::runtime_error("z-test requires --sigma (population std dev)");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::z_test(data.begin(), data.end(), FLAGS_mu0, FLAGS_sigma, alt);
        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"mu0", FLAGS_mu0}, {"sigma", FLAGS_sigma}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "f") {
        if (cols.size() != 2) {
            throw std::runtime_error("F-test requires 2 columns (--col col1,col2)");
        }
        auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
        auto r = statcpp::f_test(d1.begin(), d1.end(), d2.begin(), d2.end(), alt);
        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "shapiro") {
        if (cols.empty()) {
            throw std::runtime_error("shapiro test requires --col");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::shapiro_wilk_test(data.begin(), data.end());
        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "ks") {
        if (cols.empty()) {
            throw std::runtime_error("KS test requires --col");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto r = statcpp::ks_test_normal(data.begin(), data.end());
        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "mann-whitney") {
        if (cols.size() != 2) {
            throw std::runtime_error("Mann-Whitney test requires 2 columns (--col col1,col2)");
        }
        auto d1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto d2 = csv.get_clean_data(cols[1], FLAGS_fail_na);
        auto r = statcpp::mann_whitney_u_test(d1.begin(), d1.end(), d2.begin(), d2.end(), alt);
        fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n1", d1.size()}, {"n2", d2.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "wilcoxon") {
        if (cols.size() == 1) {
            auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
            auto r = statcpp::wilcoxon_signed_rank_test(data.begin(), data.end(), FLAGS_mu0, alt);
            fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"mu0", FLAGS_mu0}});
            print_test_result(fmt, r, FLAGS_alpha);
        } else if (cols.size() == 2) {
            // Paired Wilcoxon: compute differences then test
            auto [d1, d2] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
            std::vector<double> diffs(d1.size());
            for (std::size_t i = 0; i < d1.size(); ++i) {
                diffs[i] = d1[i] - d2[i];
            }
            auto r = statcpp::wilcoxon_signed_rank_test(diffs.begin(), diffs.end(), 0.0, alt);
            fmt.set_input_info({{"columns", {cols[0], cols[1]}}, {"n", d1.size()}, {"paired", true}});
            print_test_result(fmt, r, FLAGS_alpha);
        } else {
            throw std::runtime_error("Wilcoxon test requires 1 or 2 columns");
        }

    } else if (cmd.command == "kruskal") {
        if (cols.size() < 2) {
            throw std::runtime_error("Kruskal-Wallis test requires 2+ columns (--col c1,c2,c3)");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }
        auto r = statcpp::kruskal_wallis_test(groups);
        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"k", groups.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "levene") {
        if (cols.size() < 2) {
            throw std::runtime_error("Levene test requires 2+ columns (--col c1,c2)");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }
        auto r = statcpp::levene_test(groups);
        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"k", groups.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "bartlett") {
        if (cols.size() < 2) {
            throw std::runtime_error("Bartlett test requires 2+ columns (--col c1,c2)");
        }
        std::vector<std::vector<double>> groups;
        for (const auto& c : cols) {
            groups.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }
        auto r = statcpp::bartlett_test(groups);
        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"k", groups.size()}});
        print_test_result(fmt, r, FLAGS_alpha);

    } else if (cmd.command == "chisq") {
        if (cols.size() == 1) {
            // Uniform goodness-of-fit
            auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
            // Convert to integer counts
            std::vector<double> observed(data.begin(), data.end());
            auto r = statcpp::chisq_test_gof_uniform(observed.begin(), observed.end());
            fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}});
            print_test_result(fmt, r, FLAGS_alpha);
        } else if (cols.size() == 2) {
            // Goodness-of-fit with expected values
            auto obs = csv.get_clean_data(cols[0], FLAGS_fail_na);
            auto exp = csv.get_clean_data(cols[1], FLAGS_fail_na);
            auto r = statcpp::chisq_test_gof(obs.begin(), obs.end(), exp.begin(), exp.end());
            fmt.set_input_info({{"observed", cols[0]}, {"expected", cols[1]}});
            print_test_result(fmt, r, FLAGS_alpha);
        } else {
            throw std::runtime_error("chi-square test requires 1 or 2 columns");
        }

    } else {
        throw std::runtime_error("Unknown test command: " + cmd.command);
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
