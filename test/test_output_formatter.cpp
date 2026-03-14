/**
 * @file test_output_formatter.cpp
 * @brief Unit tests for the OutputFormatter class.
 *
 * Verifies correct behaviour of the three output modes:
 *   - @c Text  -- human-readable labelled output to stdout.
 *   - @c Json  -- structured JSON output including command, input, and result fields.
 *   - @c Quiet -- bare numeric values only, one per line.
 *
 * Each test captures stdout via a helper RAII class and inspects the
 * resulting string for the expected content.
 */

#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include "output_formatter.hpp"

using namespace statcpp_cli;

/**
 * @brief RAII helper that redirects @c std::cout to an internal buffer.
 *
 * On construction the original @c std::cout stream-buffer is saved and
 * replaced with an internal @c std::stringstream.  On destruction the
 * original buffer is restored, so that later output is unaffected.
 */
class CaptureStdout {
public:
    /// @brief Redirect @c std::cout into an internal string buffer.
    CaptureStdout() {
        old_ = std::cout.rdbuf(buffer_.rdbuf());
    }
    /// @brief Restore the original @c std::cout stream-buffer.
    ~CaptureStdout() {
        std::cout.rdbuf(old_);
    }
    /// @brief Return the captured output as a string.
    std::string get() const { return buffer_.str(); }
private:
    std::stringstream buffer_;   ///< Internal buffer that receives redirected output.
    std::streambuf* old_;        ///< Saved original stream-buffer of @c std::cout.
};

// --- Text Mode ---

/// @brief Verify that Text mode prints the label and numeric value to stdout.
TEST(OutputFormatterTest, TextSingleValue) {
    CaptureStdout cap;
    OutputFormatter fmt(OutputFormatter::Mode::Text);
    fmt.print("Mean", 42.5);
    std::string output = cap.get();
    EXPECT_NE(output.find("Mean"), std::string::npos);
    EXPECT_NE(output.find("42.5"), std::string::npos);
}

/// @brief Verify that Text mode prints multiple labelled values to stdout.
TEST(OutputFormatterTest, TextMultipleValues) {
    CaptureStdout cap;
    OutputFormatter fmt(OutputFormatter::Mode::Text);
    fmt.print({{"Min", 10.0}, {"Max", 50.0}});
    std::string output = cap.get();
    EXPECT_NE(output.find("Min"), std::string::npos);
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("Max"), std::string::npos);
    EXPECT_NE(output.find("50"), std::string::npos);
}

// --- JSON Mode ---

/// @brief Verify that Json mode emits valid JSON with command, input, and result fields.
TEST(OutputFormatterTest, JsonFormat) {
    CaptureStdout cap;
    OutputFormatter fmt(OutputFormatter::Mode::Json);
    fmt.set_command("desc.mean");
    fmt.set_input_info({{"column", "value"}, {"n", 5}});
    fmt.print("Mean", 30.0);
    fmt.flush();
    std::string output = cap.get();

    // Parse as JSON to verify validity
    auto j = nlohmann::json::parse(output);
    EXPECT_EQ(j["command"], "desc.mean");
    EXPECT_EQ(j["input"]["column"], "value");
    EXPECT_EQ(j["input"]["n"], 5);
    EXPECT_DOUBLE_EQ(j["result"]["Mean"].get<double>(), 30.0);
}

// --- Quiet Mode ---

/// @brief Verify that Quiet mode emits only the bare numeric value followed by a newline.
TEST(OutputFormatterTest, QuietSingleValue) {
    CaptureStdout cap;
    OutputFormatter fmt(OutputFormatter::Mode::Quiet);
    fmt.print("Mean", 42.5);
    std::string output = cap.get();
    EXPECT_EQ(output, "42.5\n");
}

/// @brief Verify that Quiet mode emits multiple bare values, one per line, without labels.
TEST(OutputFormatterTest, QuietMultipleValues) {
    CaptureStdout cap;
    OutputFormatter fmt(OutputFormatter::Mode::Quiet);
    fmt.print({{"Min", 10.0}, {"Max", 50.0}});
    std::string output = cap.get();
    EXPECT_EQ(output, "10\n50\n");
}
