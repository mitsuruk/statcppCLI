/**
 * @file output_formatter.hpp
 * @brief Provides the OutputFormatter class for formatting and outputting
 *        statistical computation results in text, JSON, or quiet mode.
 */

#pragma once

#include <iomanip>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace statcpp_cli {

using json = nlohmann::json;

/**
 * @class OutputFormatter
 * @brief Formats and outputs statistical results according to a selected output mode.
 *
 * OutputFormatter supports three output modes: human-readable text, structured JSON,
 * and quiet (values only). In JSON mode, results are accumulated internally and
 * emitted as a single JSON document when flush() is called.
 */
class OutputFormatter {
public:
    /**
     * @enum Mode
     * @brief Enumerates the supported output formats.
     */
    enum class Mode {
        Text,  ///< Human-readable text with aligned labels and values.
        Json,  ///< Structured JSON output (results are buffered until flush).
        Quiet  ///< Minimal output containing only the numeric values.
    };

    /**
     * @brief Constructs an OutputFormatter with the specified output mode.
     * @param mode The output mode to use for all subsequent print operations.
     */
    explicit OutputFormatter(Mode mode) : mode_(mode) {}

    /**
     * @brief Outputs a single labeled result value.
     *
     * In Text mode the label and value are printed as a formatted line to stdout.
     * In Json mode the key-value pair is accumulated in the internal JSON result object.
     * In Quiet mode only the numeric value is printed.
     *
     * @param label The descriptive label for the result (e.g. "mean", "stddev").
     * @param value The numeric result value.
     */
    void print(const std::string& label, double value) {
        switch (mode_) {
            case Mode::Text:
                std::cout << "  " << std::left << std::setw(14) << (label + ":")
                          << std::right << value << std::endl;
                break;
            case Mode::Json:
                json_result_[label] = value;
                break;
            case Mode::Quiet:
                std::cout << value << std::endl;
                break;
        }
    }

    /**
     * @brief Outputs multiple labeled result values.
     *
     * Iterates over the provided vector and delegates each entry to
     * print(const std::string&, double).
     *
     * @param results A vector of (label, value) pairs to output.
     */
    void print(const std::vector<std::pair<std::string, double>>& results) {
        for (const auto& [label, value] : results) {
            print(label, value);
        }
    }

    /**
     * @brief Sets the command name stored in the JSON output.
     * @param command The command name string (e.g. "desc", "ttest").
     */
    void set_command(const std::string& command) {
        json_command_ = command;
    }

    /**
     * @brief Sets the input metadata included in the JSON output.
     * @param info A JSON object describing the input data (e.g. source file, column).
     */
    void set_input_info(const json& info) {
        json_input_ = info;
    }

    /**
     * @brief Returns the current output mode.
     * @return The Mode value that was set at construction.
     */
    Mode get_mode() const { return mode_; }

    /**
     * @brief Directly prints a fully-formed JSON document to stdout.
     *
     * This method constructs a JSON object containing the given command, input,
     * and result fields, then writes it to stdout with 4-space indentation.
     * Unlike flush(), this does not use internally accumulated state.
     *
     * @param command The command name to include in the output.
     * @param input   A JSON object describing the input parameters.
     * @param result  A JSON object containing the computation results.
     */
    void print_json(const std::string& command, const json& input, const json& result) {
        json output;
        output["command"] = command;
        output["input"] = input;
        output["result"] = result;
        std::cout << output.dump(4) << std::endl;
    }

    /**
     * @brief Flushes accumulated results as a JSON document to stdout.
     *
     * In JSON mode, this writes a JSON object containing the command name,
     * optional input metadata, and all accumulated result key-value pairs.
     * Has no effect in Text or Quiet mode.
     */
    void flush() {
        if (mode_ == Mode::Json) {
            json output;
            output["command"] = json_command_;
            if (!json_input_.is_null()) {
                output["input"] = json_input_;
            }
            output["result"] = json_result_;
            std::cout << output.dump(4) << std::endl;
        }
    }

private:
    Mode mode_;                  ///< The active output mode.
    std::string json_command_;   ///< The command name for JSON output.
    json json_input_;            ///< Input metadata for JSON output.
    json json_result_;           ///< Accumulated result key-value pairs for JSON output.
};

}  // namespace statcpp_cli
