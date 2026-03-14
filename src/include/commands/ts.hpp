/**
 * @file ts.hpp
 * @brief Time series command handler.
 *
 * Provides the implementation for time series analysis sub-commands:
 * ACF (autocorrelation), PACF (partial autocorrelation), moving average (MA),
 * exponential moving average (EMA), differencing, and forecast accuracy
 * metrics (MAE, RMSE, MAPE).
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
 * @brief Execute a time series sub-command.
 *
 * Dispatches to the appropriate time series routine based on the sub-command
 * name stored in @p cmd.  Single-column sub-commands (acf, pacf, ma, ema,
 * diff) read one column specified via @c --col, while forecast-accuracy
 * sub-commands (mae, rmse, mape) require two columns (@c --col actual,predicted).
 *
 * @param cmd  Parsed command whose @c command field selects the sub-command
 *             (acf, pacf, ma, ema, diff, mae, rmse, mape).
 * @param csv  CSV data source from which column values are retrieved.
 * @param fmt  Output formatter used to print results and metadata.
 * @return 0 on successful execution.
 * @throws std::runtime_error If the wrong number of columns is specified for
 *         the chosen sub-command, or if the sub-command name is unknown.
 */
inline int run_ts(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("ts." + cmd.command);

    if (cmd.command == "acf") {
        if (cols.size() != 1) {
            throw std::runtime_error("acf requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        std::size_t max_lag = std::min(data.size() - 1, static_cast<std::size_t>(20));
        auto acf_vals = statcpp::acf(data.begin(), data.end(), max_lag);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"max_lag", max_lag}});

        for (std::size_t i = 0; i <= max_lag; ++i) {
            fmt.print("Lag " + std::to_string(i), acf_vals[i]);
        }

    } else if (cmd.command == "pacf") {
        if (cols.size() != 1) {
            throw std::runtime_error("pacf requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        std::size_t max_lag = std::min(data.size() - 1, static_cast<std::size_t>(20));
        auto pacf_vals = statcpp::pacf(data.begin(), data.end(), max_lag);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"max_lag", max_lag}});

        for (std::size_t i = 0; i <= max_lag; ++i) {
            fmt.print("Lag " + std::to_string(i), pacf_vals[i]);
        }

    } else if (cmd.command == "ma") {
        if (cols.size() != 1) {
            throw std::runtime_error("ma requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        // Default window = 3
        std::size_t window = 3;
        auto ma_vals = statcpp::moving_average(data.begin(), data.end(), window);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"window", window}});

        for (std::size_t i = 0; i < ma_vals.size(); ++i) {
            fmt.print("MA[" + std::to_string(i + window) + "]", ma_vals[i]);
        }

    } else if (cmd.command == "ema") {
        if (cols.size() != 1) {
            throw std::runtime_error("ema requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        // Default alpha = 0.3
        double alpha = 0.3;
        auto ema_vals = statcpp::exponential_moving_average(data.begin(), data.end(), alpha);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"alpha", alpha}});

        for (std::size_t i = 0; i < ema_vals.size(); ++i) {
            fmt.print("EMA[" + std::to_string(i + 1) + "]", ema_vals[i]);
        }

    } else if (cmd.command == "diff") {
        if (cols.size() != 1) {
            throw std::runtime_error("diff requires 1 column");
        }
        auto data = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto diff_vals = statcpp::diff(data.begin(), data.end(), 1);

        fmt.set_input_info({{"column", cols[0]}, {"n", data.size()}, {"order", 1}});

        for (std::size_t i = 0; i < diff_vals.size(); ++i) {
            fmt.print("d[" + std::to_string(i + 1) + "]", diff_vals[i]);
        }

    } else if (cmd.command == "mae") {
        // Forecast accuracy: --col actual,predicted
        if (cols.size() != 2) {
            throw std::runtime_error("mae requires 2 columns (--col actual,predicted)");
        }
        auto [actual, predicted] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        double val = statcpp::mae(actual.begin(), actual.end(), predicted.begin());

        fmt.set_input_info({{"actual", cols[0]}, {"predicted", cols[1]}, {"n", actual.size()}});
        fmt.print("MAE", val);

    } else if (cmd.command == "rmse") {
        if (cols.size() != 2) {
            throw std::runtime_error("rmse requires 2 columns (--col actual,predicted)");
        }
        auto [actual, predicted] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        double val = statcpp::rmse(actual.begin(), actual.end(), predicted.begin());

        fmt.set_input_info({{"actual", cols[0]}, {"predicted", cols[1]}, {"n", actual.size()}});
        fmt.print("RMSE", val);

    } else if (cmd.command == "mape") {
        if (cols.size() != 2) {
            throw std::runtime_error("mape requires 2 columns (--col actual,predicted)");
        }
        auto [actual, predicted] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        double val = statcpp::mape(actual.begin(), actual.end(), predicted.begin());

        fmt.set_input_info({{"actual", cols[0]}, {"predicted", cols[1]}, {"n", actual.size()}});
        fmt.print("MAPE (%)", val);

    } else {
        throw std::runtime_error("Unknown ts command: " + cmd.command +
            "\nAvailable: acf, pacf, ma, ema, diff, mae, rmse, mape");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
