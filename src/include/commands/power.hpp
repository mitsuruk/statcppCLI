/**
 * @file power.hpp
 * @brief Power analysis command handler for one-sample t-test, two-sample t-test,
 *        and proportion test.
 *
 * Provides sub-commands (@c t-one, @c t-two, @c prop) that either compute
 * statistical power for a given sample size or determine the required sample
 * size to achieve a target power, depending on the flags supplied by the user.
 */

#pragma once

#include <string>
#include <vector>

#include <gflags/gflags.h>
#include <statcpp/statcpp.hpp>

#include "cli_parser.hpp"
#include "csv_reader.hpp"
#include "output_formatter.hpp"

DECLARE_double(alpha);
DECLARE_string(alternative);
DECLARE_double(effect);
DECLARE_int64(n);
DECLARE_double(power);
DECLARE_double(ratio);
DECLARE_double(p1);
DECLARE_double(p2);

namespace statcpp_cli {
namespace commands {

/**
 * @brief Parse a string representation of the alternative hypothesis for power analysis.
 *
 * Accepted values are @c "two-sided" (or @c "two.sided"), @c "less", and @c "greater".
 *
 * @param alt  String specifying the alternative hypothesis
 *             (e.g. "two-sided", "two.sided", "less", "greater").
 * @return The corresponding @c statcpp::alternative_hypothesis enumerator.
 * @throws std::runtime_error If @p alt does not match any known alternative hypothesis.
 */
inline statcpp::alternative_hypothesis parse_alt_for_power(const std::string& alt) {
    if (alt == "two-sided" || alt == "two.sided") return statcpp::alternative_hypothesis::two_sided;
    if (alt == "less") return statcpp::alternative_hypothesis::less;
    if (alt == "greater") return statcpp::alternative_hypothesis::greater;
    throw std::runtime_error("Unknown alternative: " + alt);
}

/**
 * @brief Execute a power analysis sub-command.
 *
 * Dispatches to one of three analyses based on @c cmd.command:
 *   - @c "t-one"  -- One-sample t-test power / sample-size calculation.
 *   - @c "t-two"  -- Two-sample t-test power / sample-size calculation.
 *   - @c "prop"   -- Two-proportion test power / sample-size calculation.
 *
 * When the @c --n flag is provided (positive), the function computes statistical
 * power for the given sample size. Otherwise it computes the required sample size
 * to achieve the target power specified by @c --power.
 *
 * @param cmd  Parsed command containing the sub-command name (e.g. "t-one").
 * @param csv  CSV data (unused by this command; reserved for interface consistency).
 * @param fmt  Output formatter used to display inputs and results.
 * @return 0 on success.
 * @throws std::runtime_error If the effect size or proportions are invalid,
 *         or the sub-command name is unrecognised.
 */
inline int run_power(const ParsedCommand& cmd, CsvData& /*csv*/, OutputFormatter& fmt) {
    fmt.set_command("power." + cmd.command);

    auto alt = parse_alt_for_power(FLAGS_alternative);

    if (cmd.command == "t-one") {
        if (FLAGS_effect <= 0) {
            throw std::runtime_error("power t-one requires --effect (Cohen's d > 0)");
        }

        if (FLAGS_n > 0) {
            double pwr = statcpp::power_t_test_one_sample(
                FLAGS_effect, static_cast<std::size_t>(FLAGS_n), FLAGS_alpha, alt);
            fmt.set_input_info({{"effect", FLAGS_effect}, {"n", FLAGS_n},
                              {"alpha", FLAGS_alpha}, {"alternative", FLAGS_alternative}});
            fmt.print("Power", pwr);
        } else {
            auto n_req = statcpp::sample_size_t_test_one_sample(
                FLAGS_effect, FLAGS_power, FLAGS_alpha, alt);
            fmt.set_input_info({{"effect", FLAGS_effect}, {"power", FLAGS_power},
                              {"alpha", FLAGS_alpha}, {"alternative", FLAGS_alternative}});
            fmt.print("Sample size", static_cast<double>(n_req));
        }

    } else if (cmd.command == "t-two") {
        if (FLAGS_effect <= 0) {
            throw std::runtime_error("power t-two requires --effect (Cohen's d > 0)");
        }

        if (FLAGS_n > 0) {
            auto n1 = static_cast<std::size_t>(FLAGS_n);
            auto n2 = static_cast<std::size_t>(static_cast<double>(FLAGS_n) * FLAGS_ratio);
            double pwr = statcpp::power_t_test_two_sample(
                FLAGS_effect, n1, n2, FLAGS_alpha, alt);
            fmt.set_input_info({{"effect", FLAGS_effect}, {"n1", n1}, {"n2", n2},
                              {"alpha", FLAGS_alpha}, {"alternative", FLAGS_alternative}});
            fmt.print("Power", pwr);
        } else {
            auto n_req = statcpp::sample_size_t_test_two_sample(
                FLAGS_effect, FLAGS_power, FLAGS_alpha, FLAGS_ratio, alt);
            fmt.set_input_info({{"effect", FLAGS_effect}, {"power", FLAGS_power},
                              {"alpha", FLAGS_alpha}, {"ratio", FLAGS_ratio},
                              {"alternative", FLAGS_alternative}});
            fmt.print({
                {"n1", static_cast<double>(n_req)},
                {"n2", static_cast<double>(static_cast<std::size_t>(
                    static_cast<double>(n_req) * FLAGS_ratio))},
            });
        }

    } else if (cmd.command == "prop") {
        if (FLAGS_p1 <= 0 || FLAGS_p2 <= 0) {
            throw std::runtime_error("power prop requires --p1 and --p2 (proportions in (0,1))");
        }

        if (FLAGS_n > 0) {
            double pwr = statcpp::power_prop_test(
                FLAGS_p1, FLAGS_p2, static_cast<std::size_t>(FLAGS_n), FLAGS_alpha, alt);
            fmt.set_input_info({{"p1", FLAGS_p1}, {"p2", FLAGS_p2}, {"n", FLAGS_n},
                              {"alpha", FLAGS_alpha}, {"alternative", FLAGS_alternative}});
            fmt.print("Power", pwr);
        } else {
            auto n_req = statcpp::sample_size_prop_test(
                FLAGS_p1, FLAGS_p2, FLAGS_power, FLAGS_alpha, alt);
            fmt.set_input_info({{"p1", FLAGS_p1}, {"p2", FLAGS_p2}, {"power", FLAGS_power},
                              {"alpha", FLAGS_alpha}, {"alternative", FLAGS_alternative}});
            fmt.print("Sample size", static_cast<double>(n_req));
        }

    } else {
        throw std::runtime_error("Unknown power command: " + cmd.command +
            "\nAvailable: t-one, t-two, prop");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
