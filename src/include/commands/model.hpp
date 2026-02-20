/**
 * @file model.hpp
 * @brief Model selection command handler for statcpp CLI.
 *
 * Provides the @c run_model dispatcher that handles the following sub-commands:
 *  - **aic** -- Compute AIC and BIC for a multiple linear regression model.
 *  - **cv**  -- Perform k-fold cross-validation for linear regression.
 *  - **ridge** -- Fit a ridge regression model (L2-penalized).
 *  - **lasso** -- Fit a lasso regression model (L1-penalized).
 *
 * All sub-commands expect three or more columns specified via the @c --col flag,
 * where the last column is treated as the response variable and all preceding
 * columns are treated as predictors.
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

namespace statcpp_cli {
namespace commands {

/**
 * @brief Execute a model selection / regularisation sub-command.
 *
 * Dispatches to the appropriate statistical routine based on
 * @p cmd.command and writes the results through @p fmt.
 *
 * Supported sub-commands:
 *  | Sub-command | Description                                      |
 *  |-------------|--------------------------------------------------|
 *  | `aic`       | AIC/BIC for multiple linear regression            |
 *  | `cv`        | 5-fold cross-validation (mean MSE and SE)         |
 *  | `ridge`     | Ridge regression with lambda = 1.0                |
 *  | `lasso`     | Lasso regression with lambda = 1.0                |
 *
 * @param cmd  Parsed command whose @c command field selects the sub-command
 *             (e.g. "aic", "cv", "ridge", "lasso").
 * @param csv  CSV data source from which predictor and response columns are
 *             extracted according to the @c --col flag.
 * @param fmt  Output formatter used to render results in text or JSON mode.
 *
 * @return 0 on success.
 *
 * @throws std::runtime_error If fewer than 3 columns are specified in
 *         @c --col (predictors + response).
 * @throws std::runtime_error If @p cmd.command is not one of the recognised
 *         sub-commands.
 */
inline int run_model(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("model." + cmd.command);

    // Build predictor matrix and response: last column = response
    auto build_Xy = [&]() -> std::pair<std::vector<std::vector<double>>, std::vector<double>> {
        if (cols.size() < 3) {
            throw std::runtime_error(
                "model commands require 3+ columns (--col x1,...,xp,y). Last=response.");
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

    if (cmd.command == "aic") {
        // Compute AIC/BIC for multiple regression
        auto [X, y] = build_Xy();
        auto reg = statcpp::multiple_linear_regression(X, y);
        std::size_t n = y.size();

        double aic_val = statcpp::aic_linear(reg, n);
        double bic_val = statcpp::bic_linear(reg, n);

        std::vector<std::string> x_col_names(cols.begin(), cols.end() - 1);
        nlohmann::json x_cols = x_col_names;
        fmt.set_input_info({{"predictors", x_cols}, {"response", cols.back()}, {"n", n}});
        fmt.print({
            {"AIC",           aic_val},
            {"BIC",           bic_val},
            {"R-squared",     reg.r_squared},
            {"Adj R-squared", reg.adj_r_squared},
        });

    } else if (cmd.command == "cv") {
        // k-fold cross-validation for linear regression
        auto [X, y] = build_Xy();
        std::size_t k = 5;
        auto cv = statcpp::cross_validate_linear(X, y, k);

        std::vector<std::string> x_col_names(cols.begin(), cols.end() - 1);
        nlohmann::json x_cols = x_col_names;
        fmt.set_input_info({{"predictors", x_cols}, {"response", cols.back()},
                           {"n", y.size()}, {"k", k}});
        fmt.print({
            {"Mean MSE",  cv.mean_error},
            {"SE MSE",    cv.se_error},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Fold MSEs:";
            for (std::size_t i = 0; i < cv.fold_errors.size(); ++i) {
                std::cout << " " << cv.fold_errors[i];
            }
            std::cout << std::endl;
        }

    } else if (cmd.command == "ridge") {
        auto [X, y] = build_Xy();
        double lambda = 1.0;
        auto r = statcpp::ridge_regression(X, y, lambda);

        std::vector<std::string> x_col_names(cols.begin(), cols.end() - 1);
        nlohmann::json x_cols = x_col_names;
        fmt.set_input_info({{"predictors", x_cols}, {"response", cols.back()},
                           {"lambda", lambda}});
        fmt.print("MSE", r.mse);

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Coefficients:" << std::endl;
            std::vector<std::string> x_names(cols.begin(), cols.end() - 1);
            std::cout << "  " << std::left << std::setw(16) << "(Intercept)"
                      << r.coefficients[0] << std::endl;
            for (std::size_t j = 0; j < x_names.size(); ++j) {
                std::cout << "  " << std::left << std::setw(16) << x_names[j]
                          << r.coefficients[j + 1] << std::endl;
            }
        }

    } else if (cmd.command == "lasso") {
        auto [X, y] = build_Xy();
        double lambda = 1.0;
        auto r = statcpp::lasso_regression(X, y, lambda);

        std::vector<std::string> x_col_names(cols.begin(), cols.end() - 1);
        nlohmann::json x_cols = x_col_names;
        fmt.set_input_info({{"predictors", x_cols}, {"response", cols.back()},
                           {"lambda", lambda}});
        fmt.print("MSE", r.mse);

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Coefficients:" << std::endl;
            std::vector<std::string> x_names(cols.begin(), cols.end() - 1);
            std::cout << "  " << std::left << std::setw(16) << "(Intercept)"
                      << r.coefficients[0] << std::endl;
            for (std::size_t j = 0; j < x_names.size(); ++j) {
                std::cout << "  " << std::left << std::setw(16) << x_names[j]
                          << r.coefficients[j + 1] << std::endl;
            }
        }

    } else {
        throw std::runtime_error("Unknown model command: " + cmd.command +
            "\nAvailable: aic, cv, ridge, lasso");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
