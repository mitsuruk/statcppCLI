/**
 * @file cli_parser.hpp
 * @brief Command-line argument parser for statcpp CLI.
 *
 * Provides subcommand parsing logic that maps user input to a category/command
 * pair, supporting both explicit `<category> <command>` syntax and shortcut
 * aliases (e.g., "mean" -> desc/mean).
 */

#pragma once

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace statcpp_cli {

/**
 * @brief Holds the parsed result of a CLI invocation.
 */
struct ParsedCommand {
    std::string category;  ///< Command category (e.g., "desc", "test", "corr").
    std::string command;   ///< Subcommand within the category (e.g., "mean", "t").
    std::string filepath;  ///< Path to the input CSV file; empty when reading from stdin.
};

/**
 * @brief Parses CLI arguments into a ParsedCommand.
 *
 * After gflags has consumed its flags, the remaining positional arguments are
 * interpreted in one of two patterns:
 *   - `statcpp <category> <command> [file]`
 *   - `statcpp <shortcut> [file]`
 *
 * Shortcuts are expanded to their corresponding (category, command) pair.
 *
 * @param argc Argument count (post-gflags parsing).
 * @param argv Argument vector (post-gflags parsing).
 * @return A ParsedCommand with category, command, and optional filepath.
 * @throws std::runtime_error If no arguments are given, the command is missing,
 *         or the first argument is not a known category or shortcut.
 */
inline ParsedCommand parse_command(int argc, char* argv[]) {
    // After gflags parsing, remaining args are non-flag arguments.
    // argv[0] = program name
    // Possible patterns:
    //   statcpp <category> <command> [file]
    //   statcpp <shortcut> [file]

    // Shortcut -> (category, command)
    static const std::unordered_map<std::string, std::pair<std::string, std::string>>
        shortcuts = {
            {"mean",      {"desc", "mean"}},
            {"median",    {"desc", "median"}},
            {"mode",      {"desc", "mode"}},
            {"sd",        {"desc", "sd"}},
            {"var",       {"desc", "var"}},
            {"summary",   {"desc", "summary"}},
            {"range",     {"desc", "range"}},
            {"iqr",       {"desc", "iqr"}},
            {"cv",        {"desc", "cv"}},
            {"skewness",  {"desc", "skewness"}},
            {"kurtosis",  {"desc", "kurtosis"}},
            {"quartiles", {"desc", "quartiles"}},
            {"gmean",     {"desc", "gmean"}},
            {"hmean",     {"desc", "hmean"}},
            // Phase 2 shortcuts
            {"ttest",     {"test", "t"}},
            {"pearson",   {"corr", "pearson"}},
            {"spearman",  {"corr", "spearman"}},
            {"kendall",   {"corr", "kendall"}},
        };

    // Known categories
    static const std::vector<std::string> categories = {
        "desc", "corr", "test", "effect", "ci", "reg",
        "anova", "resample", "ts", "robust", "survival",
        "cluster", "multiple", "power", "glm", "model"
    };

    // Collect non-flag args (skip argv[0])
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.push_back(argv[i]);
    }

    if (args.empty()) {
        throw std::runtime_error(
            "Usage: statcpp <category> <command> [options] [file]\n"
            "       statcpp <shortcut> [options] [file]\n"
            "Run 'statcpp --help' for more information.");
    }

    ParsedCommand result;

    // Check if first arg is a known category
    bool is_category = false;
    for (const auto& cat : categories) {
        if (args[0] == cat) {
            is_category = true;
            break;
        }
    }

    if (is_category) {
        result.category = args[0];
        if (args.size() < 2) {
            throw std::runtime_error(
                "Missing command. Usage: statcpp " + result.category + " <command> [file]");
        }
        result.command = args[1];
        if (args.size() >= 3) {
            result.filepath = args[2];
        }
    } else {
        // Try shortcut
        auto it = shortcuts.find(args[0]);
        if (it != shortcuts.end()) {
            result.category = it->second.first;
            result.command = it->second.second;
            if (args.size() >= 2) {
                result.filepath = args[1];
            }
        } else {
            throw std::runtime_error("Unknown command: " + args[0] +
                "\nRun 'statcpp --help' for available commands.");
        }
    }

    return result;
}

}  // namespace statcpp_cli
