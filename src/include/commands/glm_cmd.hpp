/**
 * @file glm_cmd.hpp
 * @brief Command handler for generalized linear models (GLM).
 *
 * Provides the CLI entry point for fitting generalized linear models,
 * currently supporting logistic regression (binomial family) and
 * Poisson regression (Poisson family). The handler parses column
 * specifications, constructs the predictor matrix and response vector
 * from CSV data, delegates to the statcpp library for model fitting,
 * and formats the results for output.
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

namespace statcpp_cli {
namespace commands {

/**
 * @brief Execute a generalized linear model sub-command.
 *
 * Reads the columns specified by the @c --col flag, treats the last column as
 * the response variable and all preceding columns as predictors, then fits the
 * GLM indicated by @p cmd.command. Supported sub-commands are:
 *
 * - @c "logistic" -- Logistic regression (binomial family with logit link).
 *   Prints deviance statistics, coefficient table, McFadden pseudo R-squared,
 *   and odds ratios.
 * - @c "poisson" -- Poisson regression (Poisson family with log link).
 *   Prints deviance statistics, coefficient table, and incidence rate ratios.
 *
 * @param cmd  Parsed command structure whose @c command field selects the GLM
 *             family (e.g. "logistic" or "poisson").
 * @param csv  CSV data source from which predictor and response columns are
 *             extracted.
 * @param fmt  Output formatter that controls whether results are emitted as
 *             human-readable text or structured JSON.
 * @return 0 on successful completion.
 * @throws std::runtime_error If fewer than two columns are specified in
 *         @c --col (GLM requires at least one predictor and one response).
 * @throws std::runtime_error If @p cmd.command is not a recognised GLM
 *         sub-command.
 */
inline int run_glm(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("glm." + cmd.command);

    // Build predictor matrix and response: last column = response
    auto build_Xy = [&]() -> std::pair<std::vector<std::vector<double>>, std::vector<double>> {
        if (cols.size() < 2) {
            throw std::runtime_error("GLM requires 2+ columns (--col x1,...,xp,y). Last=response.");
        }
        std::string y_col = cols.back();
        auto ydata = csv.get_clean_data(y_col, FLAGS_fail_na);
        std::size_t n = ydata.size();
        std::size_t p = cols.size() - 1;

        std::vector<std::vector<double>> X_cols;
        for (std::size_t j = 0; j < p; ++j) {
            X_cols.push_back(csv.get_clean_data(cols[j], FLAGS_fail_na));
        }

        std::vector<std::vector<double>> X(n, std::vector<double>(p));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                X[i][j] = X_cols[j][i];
            }
        }
        return {X, ydata};
    };

    auto print_glm_result = [&](const statcpp::glm_result& r, const std::vector<std::string>& x_cols) {
        fmt.print({
            {"Null deviance",     r.null_deviance},
            {"Residual deviance", r.residual_deviance},
            {"AIC",               r.aic},
            {"Log-likelihood",    r.log_likelihood},
            {"Iterations",        static_cast<double>(r.iterations)},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Coefficients:" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << ""
                      << std::setw(12) << "Estimate"
                      << std::setw(12) << "Std.Error"
                      << std::setw(12) << "z-value"
                      << "p-value" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << "(Intercept)"
                      << std::setw(12) << r.coefficients[0]
                      << std::setw(12) << r.coefficient_se[0]
                      << std::setw(12) << r.z_statistics[0]
                      << r.p_values[0] << std::endl;
            for (std::size_t j = 0; j < x_cols.size(); ++j) {
                std::cout << "  " << std::left << std::setw(16) << x_cols[j]
                          << std::setw(12) << r.coefficients[j + 1]
                          << std::setw(12) << r.coefficient_se[j + 1]
                          << std::setw(12) << r.z_statistics[j + 1]
                          << r.p_values[j + 1] << std::endl;
            }
        }
    };

    if (cmd.command == "logistic") {
        auto [X, y] = build_Xy();
        std::vector<std::string> x_cols(cols.begin(), cols.end() - 1);

        auto r = statcpp::logistic_regression(X, y);

        nlohmann::json x_names = x_cols;
        fmt.set_input_info({{"predictors", x_names}, {"response", cols.back()},
                           {"n", y.size()}, {"family", "binomial"}});
        print_glm_result(r, x_cols);

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Pseudo R-squared (McFadden): "
                      << statcpp::pseudo_r_squared_mcfadden(r) << std::endl;
            auto or_vals = statcpp::odds_ratios(r);
            std::cout << "  Odds ratios:";
            for (std::size_t j = 0; j < x_cols.size(); ++j) {
                std::cout << " " << x_cols[j] << "=" << or_vals[j + 1];
            }
            std::cout << std::endl;
        }

    } else if (cmd.command == "poisson") {
        auto [X, y] = build_Xy();
        std::vector<std::string> x_cols(cols.begin(), cols.end() - 1);

        auto r = statcpp::poisson_regression(X, y);

        nlohmann::json x_names = x_cols;
        fmt.set_input_info({{"predictors", x_names}, {"response", cols.back()},
                           {"n", y.size()}, {"family", "poisson"}});
        print_glm_result(r, x_cols);

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            auto irr = statcpp::incidence_rate_ratios(r);
            std::cout << "\n  Incidence rate ratios:";
            for (std::size_t j = 0; j < x_cols.size(); ++j) {
                std::cout << " " << x_cols[j] << "=" << irr[j + 1];
            }
            std::cout << std::endl;
        }

    } else {
        throw std::runtime_error("Unknown glm command: " + cmd.command +
            "\nAvailable: logistic, poisson");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
