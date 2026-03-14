/**
 * @file reg.hpp
 * @brief Regression command handler for the statcpp CLI.
 *
 * Provides the entry point for all regression-related subcommands:
 * - @c simple   -- Simple linear regression (one predictor, one response).
 * - @c multiple -- Multiple linear regression (two or more predictors, one response).
 * - @c predict  -- Point prediction with prediction interval from a simple linear model.
 * - @c residuals -- Residual diagnostics (Durbin-Watson, leverage, Cook's distance).
 * - @c vif      -- Variance Inflation Factor for multicollinearity detection.
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
 * @brief Execute a regression subcommand.
 *
 * Dispatches to the appropriate regression routine based on
 * @p cmd.command and writes the results through @p fmt.
 *
 * Supported subcommands:
 * | Subcommand   | Required @c --col layout             | Description                                    |
 * |--------------|--------------------------------------|------------------------------------------------|
 * | @c simple    | @c x,y (2 columns)                   | Simple linear regression (x = predictor, y = response). |
 * | @c multiple  | @c x1,x2,...,y (3+ columns)          | Multiple linear regression; last column is the response. |
 * | @c predict   | @c x,y (2 columns)                   | Predict the response for the last observed x value, with prediction interval at the confidence level given by @c --level. |
 * | @c residuals | @c x,y (2 columns)                   | Compute residual diagnostics (Durbin-Watson statistic, standardized residuals, leverage, Cook's distance). |
 * | @c vif       | @c x1,x2,... (2+ predictor columns)  | Compute Variance Inflation Factors; warns when VIF > 10. |
 *
 * @param cmd  Parsed command whose @c command field selects the subcommand.
 * @param csv  CSV data source from which column data is extracted.
 * @param fmt  Output formatter that receives and renders the results
 *             (text or JSON depending on the current mode).
 *
 * @return 0 on successful completion.
 *
 * @throws std::runtime_error If the wrong number of columns is supplied for
 *         the chosen subcommand, if column lengths are mismatched, or if
 *         an unknown subcommand is requested.
 */
inline int run_reg(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("reg." + cmd.command);

    if (cmd.command == "simple") {
        // Simple linear regression: --col x,y (first=predictor, second=response)
        if (cols.size() != 2) {
            throw std::runtime_error("simple regression requires 2 columns (--col x,y)");
        }
        auto [xdata, ydata] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        auto r = statcpp::simple_linear_regression(
            xdata.begin(), xdata.end(), ydata.begin(), ydata.end());

        fmt.set_input_info({{"x", cols[0]}, {"y", cols[1]}, {"n", xdata.size()}});
        fmt.print({
            {"Intercept",    r.intercept},
            {"Slope",        r.slope},
            {"R-squared",    r.r_squared},
            {"Adj R-squared", r.adj_r_squared},
            {"Residual SE",  r.residual_se},
            {"F-statistic",  r.f_statistic},
            {"F p-value",    r.f_p_value},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Coefficients:" << std::endl;
            std::cout << "  " << std::left << std::setw(14) << ""
                      << std::setw(12) << "Estimate"
                      << std::setw(12) << "Std.Error"
                      << std::setw(12) << "t-value"
                      << "p-value" << std::endl;
            std::cout << "  " << std::left << std::setw(14) << "Intercept"
                      << std::setw(12) << r.intercept
                      << std::setw(12) << r.intercept_se
                      << std::setw(12) << r.intercept_t
                      << r.intercept_p << std::endl;
            std::cout << "  " << std::left << std::setw(14) << "Slope"
                      << std::setw(12) << r.slope
                      << std::setw(12) << r.slope_se
                      << std::setw(12) << r.slope_t
                      << r.slope_p << std::endl;
        }

    } else if (cmd.command == "multiple") {
        // Multiple regression: --col x1,x2,...,y (last column = response)
        if (cols.size() < 3) {
            throw std::runtime_error(
                "multiple regression requires 3+ columns (--col x1,x2,...,y). "
                "Last column is the response variable.");
        }
        // Last column is response
        std::string y_col = cols.back();
        std::vector<std::string> x_cols(cols.begin(), cols.end() - 1);

        // Build predictor matrix and response vector
        auto ydata = csv.get_clean_data(y_col, FLAGS_fail_na);
        std::size_t n = ydata.size();

        // Get predictor data - must match response length
        std::vector<std::vector<double>> X_cols;
        for (const auto& xc : x_cols) {
            X_cols.push_back(csv.get_clean_data(xc, FLAGS_fail_na));
        }

        // Transpose: X_cols[p][n] -> X[n][p]
        std::size_t p = x_cols.size();
        std::vector<std::vector<double>> X(n, std::vector<double>(p));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                if (i >= X_cols[j].size()) {
                    throw std::runtime_error("Column '" + x_cols[j] +
                        "' has fewer rows than response column");
                }
                X[i][j] = X_cols[j][i];
            }
        }

        auto r = statcpp::multiple_linear_regression(X, ydata);

        nlohmann::json x_names = x_cols;
        fmt.set_input_info({{"predictors", x_names}, {"response", y_col}, {"n", n}});
        fmt.print({
            {"R-squared",     r.r_squared},
            {"Adj R-squared", r.adj_r_squared},
            {"Residual SE",   r.residual_se},
            {"F-statistic",   r.f_statistic},
            {"F p-value",     r.f_p_value},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Coefficients:" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << ""
                      << std::setw(12) << "Estimate"
                      << std::setw(12) << "Std.Error"
                      << std::setw(12) << "t-value"
                      << "p-value" << std::endl;
            std::cout << "  " << std::left << std::setw(16) << "(Intercept)"
                      << std::setw(12) << r.coefficients[0]
                      << std::setw(12) << r.coefficient_se[0]
                      << std::setw(12) << r.t_statistics[0]
                      << r.p_values[0] << std::endl;
            for (std::size_t j = 0; j < p; ++j) {
                std::cout << "  " << std::left << std::setw(16) << x_cols[j]
                          << std::setw(12) << r.coefficients[j + 1]
                          << std::setw(12) << r.coefficient_se[j + 1]
                          << std::setw(12) << r.t_statistics[j + 1]
                          << r.p_values[j + 1] << std::endl;
            }
        }

    } else if (cmd.command == "predict") {
        // Predict: --col x,y (fit simple regression, predict for last x or --x0 value)
        if (cols.size() != 2) {
            throw std::runtime_error("predict requires 2 columns (--col x,y)");
        }
        auto [xdata, ydata] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        auto model = statcpp::simple_linear_regression(
            xdata.begin(), xdata.end(), ydata.begin(), ydata.end());

        // Predict for last x value
        double x_new = xdata.back();
        double y_pred = statcpp::predict(model, x_new);

        auto pi = statcpp::prediction_interval_simple(
            model, xdata.begin(), xdata.end(), x_new, FLAGS_level);

        fmt.set_input_info({{"x", cols[0]}, {"y", cols[1]},
                           {"n", xdata.size()}, {"x_new", x_new}});
        fmt.print({
            {"x_new",      x_new},
            {"Prediction", y_pred},
            {"Lower",      pi.lower},
            {"Upper",      pi.upper},
            {"SE",         pi.se_prediction},
        });

    } else if (cmd.command == "residuals") {
        if (cols.size() != 2) {
            throw std::runtime_error("residuals requires 2 columns (--col x,y)");
        }
        auto [xdata, ydata] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        auto model = statcpp::simple_linear_regression(
            xdata.begin(), xdata.end(), ydata.begin(), ydata.end());
        auto diag = statcpp::compute_residual_diagnostics(
            model, xdata.begin(), xdata.end(), ydata.begin(), ydata.end());

        fmt.set_input_info({{"x", cols[0]}, {"y", cols[1]}, {"n", xdata.size()}});
        fmt.print("Durbin-Watson", diag.durbin_watson);

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Residuals (first 10):" << std::endl;
            std::cout << "  " << std::left << std::setw(6) << "Obs"
                      << std::setw(12) << "Residual"
                      << std::setw(12) << "Std.Resid"
                      << std::setw(12) << "Leverage"
                      << "Cook's D" << std::endl;
            std::size_t show = std::min(diag.residuals.size(), static_cast<std::size_t>(10));
            for (std::size_t i = 0; i < show; ++i) {
                std::cout << "  " << std::left << std::setw(6) << (i + 1)
                          << std::setw(12) << diag.residuals[i]
                          << std::setw(12) << diag.standardized_residuals[i]
                          << std::setw(12) << diag.hat_values[i]
                          << diag.cooks_distance[i] << std::endl;
            }
        }

    } else if (cmd.command == "vif") {
        // VIF: --col x1,x2,...  (predictors only, no response)
        if (cols.size() < 2) {
            throw std::runtime_error("vif requires 2+ predictor columns (--col x1,x2,...)");
        }
        std::vector<std::vector<double>> X_cols;
        for (const auto& c : cols) {
            X_cols.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }
        std::size_t n = X_cols[0].size();
        std::size_t p = cols.size();
        std::vector<std::vector<double>> X(n, std::vector<double>(p));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                X[i][j] = X_cols[j][i];
            }
        }

        auto vif = statcpp::compute_vif(X);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"predictors", col_names}, {"n", n}});
        for (std::size_t j = 0; j < p; ++j) {
            fmt.print(cols[j], vif[j]);
        }

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            // Interpretation
            for (std::size_t j = 0; j < p; ++j) {
                if (vif[j] > 10.0) {
                    std::cout << "  WARNING: " << cols[j]
                              << " VIF > 10 (multicollinearity)" << std::endl;
                }
            }
        }

    } else {
        throw std::runtime_error("Unknown reg command: " + cmd.command +
            "\nAvailable: simple, multiple, predict, residuals, vif");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
