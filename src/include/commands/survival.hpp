/**
 * @file survival.hpp
 * @brief Survival analysis command handler for the statcpp CLI.
 *
 * Provides the entry point for survival analysis sub-commands, including
 * Kaplan-Meier survival estimation, the log-rank test for comparing two
 * survival curves, and Nelson-Aalen cumulative hazard estimation.
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
 * @brief Execute a survival analysis sub-command.
 *
 * Dispatches to the appropriate survival analysis routine based on
 * @p cmd.command:
 *
 * - **kaplan-meier** -- Computes Kaplan-Meier survival estimates.
 *   Requires two columns specified via @c --col @c time,event
 *   (event: 1 = event occurred, 0 = censored).
 *   In text mode a full survival table (time, survival probability, SE,
 *   at-risk count, events, censored) and the median survival time are
 *   printed. In structured mode the median survival time and total number
 *   of events are emitted.
 *
 * - **logrank** -- Performs a log-rank test comparing two survival curves.
 *   Requires four columns via @c --col @c time1,event1,time2,event2.
 *   Outputs the chi-square statistic, p-value, degrees of freedom, and
 *   observed / expected event counts for each group.
 *
 * - **nelson-aalen** -- Computes the Nelson-Aalen cumulative hazard
 *   estimator. Requires two columns via @c --col @c time,event.
 *   In text mode a table of time, hazard, and cumulative hazard is
 *   printed. In structured mode cumulative hazard values at each event
 *   time are emitted.
 *
 * @param cmd  Parsed command containing the sub-command name (e.g.
 *             "kaplan-meier", "logrank", "nelson-aalen") and any
 *             additional arguments.
 * @param csv  CSV data source from which columns are read. Column names
 *             are specified through the global @c --col flag.
 * @param fmt  Output formatter that controls whether results are printed
 *             as human-readable text or structured output (e.g. JSON).
 *
 * @return 0 on successful execution.
 *
 * @throws std::runtime_error If the wrong number of columns is provided
 *         for the chosen sub-command, or if an unknown sub-command name
 *         is given.
 */
inline int run_survival(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("survival." + cmd.command);

    if (cmd.command == "kaplan-meier") {
        // --col time,event  (event: 1=event, 0=censored)
        if (cols.size() != 2) {
            throw std::runtime_error(
                "kaplan-meier requires 2 columns (--col time,event). "
                "event: 1=event, 0=censored");
        }
        auto [times, events_raw] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        std::vector<bool> events;
        events.reserve(events_raw.size());
        for (double e : events_raw) {
            events.push_back(e != 0.0);
        }

        auto km = statcpp::kaplan_meier(times, events);

        fmt.set_input_info({{"time", cols[0]}, {"event", cols[1]}, {"n", times.size()}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Kaplan-Meier Survival Estimates:" << std::endl;
            std::cout << "  " << std::left << std::setw(10) << "Time"
                      << std::setw(12) << "Survival"
                      << std::setw(10) << "SE"
                      << std::setw(10) << "At Risk"
                      << std::setw(10) << "Events"
                      << "Censored" << std::endl;
            for (std::size_t i = 0; i < km.times.size(); ++i) {
                std::cout << "  " << std::left << std::setw(10) << km.times[i]
                          << std::setw(12) << km.survival[i]
                          << std::setw(10) << km.se[i]
                          << std::setw(10) << km.n_at_risk[i]
                          << std::setw(10) << km.n_events[i]
                          << km.n_censored[i] << std::endl;
            }
            double median = statcpp::median_survival_time(km);
            std::cout << "\n  Median survival: " << median << std::endl;
        } else {
            double median = statcpp::median_survival_time(km);
            fmt.print("Median survival", median);
            fmt.print("N events", static_cast<double>(
                std::count(events.begin(), events.end(), true)));
        }

    } else if (cmd.command == "logrank") {
        // --col time1,event1,time2,event2
        if (cols.size() != 4) {
            throw std::runtime_error(
                "logrank requires 4 columns (--col time1,event1,time2,event2)");
        }
        auto times1 = csv.get_clean_data(cols[0], FLAGS_fail_na);
        auto events1_raw = csv.get_clean_data(cols[1], FLAGS_fail_na);
        auto times2 = csv.get_clean_data(cols[2], FLAGS_fail_na);
        auto events2_raw = csv.get_clean_data(cols[3], FLAGS_fail_na);

        std::vector<bool> events1, events2;
        for (double e : events1_raw) events1.push_back(e != 0.0);
        for (double e : events2_raw) events2.push_back(e != 0.0);

        auto r = statcpp::logrank_test(times1, events1, times2, events2);

        fmt.set_input_info({{"group1_time", cols[0]}, {"group1_event", cols[1]},
                           {"group2_time", cols[2]}, {"group2_event", cols[3]}});
        fmt.print({
            {"Chi-square",  r.statistic},
            {"p-value",     r.p_value},
            {"df",          static_cast<double>(r.df)},
            {"Observed 1",  static_cast<double>(r.observed1)},
            {"Expected 1",  r.expected1},
            {"Observed 2",  static_cast<double>(r.observed2)},
            {"Expected 2",  r.expected2},
        });

    } else if (cmd.command == "nelson-aalen") {
        if (cols.size() != 2) {
            throw std::runtime_error(
                "nelson-aalen requires 2 columns (--col time,event)");
        }
        auto [times, events_raw] = csv.get_paired_clean_data(cols[0], cols[1], FLAGS_fail_na);
        std::vector<bool> events;
        for (double e : events_raw) events.push_back(e != 0.0);

        auto na = statcpp::nelson_aalen(times, events);

        fmt.set_input_info({{"time", cols[0]}, {"event", cols[1]}, {"n", times.size()}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Nelson-Aalen Cumulative Hazard:" << std::endl;
            std::cout << "  " << std::left << std::setw(10) << "Time"
                      << std::setw(12) << "Hazard"
                      << "Cum.Hazard" << std::endl;
            for (std::size_t i = 0; i < na.times.size(); ++i) {
                std::cout << "  " << std::left << std::setw(10) << na.times[i]
                          << std::setw(12) << na.hazard[i]
                          << na.cumulative_hazard[i] << std::endl;
            }
        } else {
            for (std::size_t i = 0; i < na.times.size(); ++i) {
                fmt.print("H(" + std::to_string(static_cast<int>(na.times[i])) + ")",
                         na.cumulative_hazard[i]);
            }
        }

    } else {
        throw std::runtime_error("Unknown survival command: " + cmd.command +
            "\nAvailable: kaplan-meier, logrank, nelson-aalen");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
