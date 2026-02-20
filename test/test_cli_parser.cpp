/**
 * @file test_cli_parser.cpp
 * @brief Unit tests for the CLI argument parser (parse_command).
 *
 * Exercises the two supported invocation patterns:
 *   - Full form:  @c statcpp @c \<category\> @c \<command\> @c [file]
 *   - Shortcut:   @c statcpp @c \<shortcut\> @c [file]
 *
 * Also verifies error handling for unknown commands, missing arguments,
 * and that every registered category string is correctly recognised.
 */

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include "cli_parser.hpp"

using namespace statcpp_cli;

/**
 * @brief Helper struct that builds an argc/argv pair from a list of strings.
 *
 * Stores the string data and provides @c argc() / @c argv() accessors
 * compatible with the signature expected by @c parse_command().
 */
struct Args {
    std::vector<std::string> strs;  ///< Owned argument strings.
    std::vector<char*> ptrs;        ///< Raw pointers into @c strs for argv.

    /**
     * @brief Construct from an initializer list of argument strings.
     * @param args  Argument strings (e.g. @c {"statcpp", "desc", "mean"}).
     */
    Args(std::initializer_list<std::string> args) : strs(args) {
        for (auto& s : strs) {
            ptrs.push_back(s.data());
        }
    }
    /// @brief Return the argument count.
    int argc() { return static_cast<int>(ptrs.size()); }
    /// @brief Return the argument vector.
    char** argv() { return ptrs.data(); }
};

// --- Full Command: category + command + file ---

/// @brief Verify parsing of the full form: category, command, and filepath.
TEST(CliParserTest, FullCommand) {
    Args a({"statcpp", "desc", "mean", "data.csv"});
    auto cmd = parse_command(a.argc(), a.argv());
    EXPECT_EQ(cmd.category, "desc");
    EXPECT_EQ(cmd.command, "mean");
    EXPECT_EQ(cmd.filepath, "data.csv");
}

// --- Full Command without file (stdin) ---

/// @brief Verify that omitting the filepath leaves it empty (stdin mode).
TEST(CliParserTest, FullCommandStdin) {
    Args a({"statcpp", "desc", "summary"});
    auto cmd = parse_command(a.argc(), a.argv());
    EXPECT_EQ(cmd.category, "desc");
    EXPECT_EQ(cmd.command, "summary");
    EXPECT_TRUE(cmd.filepath.empty());
}

// --- Shortcut ---

/// @brief Verify that the "mean" shortcut expands to category="desc", command="mean".
TEST(CliParserTest, ShortcutMean) {
    Args a({"statcpp", "mean", "data.csv"});
    auto cmd = parse_command(a.argc(), a.argv());
    EXPECT_EQ(cmd.category, "desc");
    EXPECT_EQ(cmd.command, "mean");
    EXPECT_EQ(cmd.filepath, "data.csv");
}

/// @brief Verify that the "sd" shortcut expands to category="desc", command="sd".
TEST(CliParserTest, ShortcutSd) {
    Args a({"statcpp", "sd", "data.csv"});
    auto cmd = parse_command(a.argc(), a.argv());
    EXPECT_EQ(cmd.category, "desc");
    EXPECT_EQ(cmd.command, "sd");
}

/// @brief Verify that the "summary" shortcut works without a filepath argument.
TEST(CliParserTest, ShortcutSummary) {
    Args a({"statcpp", "summary"});
    auto cmd = parse_command(a.argc(), a.argv());
    EXPECT_EQ(cmd.category, "desc");
    EXPECT_EQ(cmd.command, "summary");
    EXPECT_TRUE(cmd.filepath.empty());
}

// --- Unknown Command ---

/// @brief Verify that an unrecognised command throws a runtime error.
TEST(CliParserTest, UnknownCommand) {
    Args a({"statcpp", "foobar"});
    EXPECT_THROW(parse_command(a.argc(), a.argv()), std::runtime_error);
}

// --- No Arguments ---

/// @brief Verify that invoking with no positional arguments throws a runtime error.
TEST(CliParserTest, NoArgs) {
    Args a({"statcpp"});
    EXPECT_THROW(parse_command(a.argc(), a.argv()), std::runtime_error);
}

// --- Missing Command after Category ---

/// @brief Verify that specifying a category without a command throws a runtime error.
TEST(CliParserTest, MissingCommand) {
    Args a({"statcpp", "desc"});
    EXPECT_THROW(parse_command(a.argc(), a.argv()), std::runtime_error);
}

// --- All Categories Recognized ---

/// @brief Verify that every registered category string is accepted by the parser.
TEST(CliParserTest, AllCategories) {
    for (const auto& cat : {"desc", "corr", "test", "effect", "ci", "reg",
                            "anova", "resample", "ts", "robust", "survival",
                            "cluster", "multiple"}) {
        Args a({"statcpp", cat, "cmd"});
        auto cmd = parse_command(a.argc(), a.argv());
        EXPECT_EQ(cmd.category, cat);
        EXPECT_EQ(cmd.command, "cmd");
    }
}
