/**
 * @file test_csv_reader.cpp
 * @brief Unit tests for the CsvReader and CsvData classes.
 *
 * Validates CSV parsing behaviour including basic comma-separated input,
 * RFC 4180 quoted fields, missing-value recognition (NA, NaN, empty),
 * tab-delimited auto-detection, Windows CRLF line endings, UTF-8 BOM
 * stripping, header-less numeric input, column lookup by name and index,
 * error handling for missing columns and empty input, and trailing-comma
 * edge cases.
 */

#include <gtest/gtest.h>
#include <sstream>
#include <cmath>
#include "csv_reader.hpp"

using namespace statcpp_cli;

// --- Basic CSV ---

/// @brief Verify that a simple 3-column CSV is parsed into named columns with correct values.
TEST(CsvReaderTest, BasicCsv) {
    std::istringstream input("name,value,score\nA,10,85.5\nB,20,90.0\nC,30,75.5\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    ASSERT_EQ(csv.columns.size(), 3u);
    EXPECT_EQ(csv.columns[0].name, "name");
    EXPECT_EQ(csv.columns[1].name, "value");
    EXPECT_EQ(csv.columns[2].name, "score");

    ASSERT_EQ(csv.columns[1].data.size(), 3u);
    EXPECT_DOUBLE_EQ(csv.columns[1].data[0], 10.0);
    EXPECT_DOUBLE_EQ(csv.columns[1].data[1], 20.0);
    EXPECT_DOUBLE_EQ(csv.columns[1].data[2], 30.0);

    EXPECT_DOUBLE_EQ(csv.columns[2].data[0], 85.5);
    EXPECT_DOUBLE_EQ(csv.columns[2].data[2], 75.5);
}

// --- Quoted Fields (RFC 4180) ---

/// @brief Verify RFC 4180 quoted-field handling: embedded commas and escaped double-quotes.
TEST(CsvReaderTest, QuotedFields) {
    std::istringstream input("name,city,value\n\"Smith, John\",\"New York\",42.5\n\"O\"\"Brien\",Tokyo,10.0\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    ASSERT_EQ(csv.columns.size(), 3u);
    // "Smith, John" has comma inside quotes - should not split
    ASSERT_EQ(csv.columns[0].data.size(), 2u);
    // name column is non-numeric, stored as NaN
    EXPECT_TRUE(std::isnan(csv.columns[0].data[0]));
    EXPECT_DOUBLE_EQ(csv.columns[2].data[0], 42.5);
    EXPECT_DOUBLE_EQ(csv.columns[2].data[1], 10.0);
}

// --- Missing Values ---

/// @brief Verify that NA, empty strings, and NaN are all recognised as missing (stored as NaN).
TEST(CsvReaderTest, MissingValues) {
    std::istringstream input("id,value\n1,10\n2,NA\n3,\n4,NaN\n5,50\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    ASSERT_EQ(csv.columns[1].data.size(), 5u);
    EXPECT_DOUBLE_EQ(csv.columns[1].data[0], 10.0);
    EXPECT_TRUE(std::isnan(csv.columns[1].data[1]));  // NA
    EXPECT_TRUE(std::isnan(csv.columns[1].data[2]));  // empty
    EXPECT_TRUE(std::isnan(csv.columns[1].data[3]));  // NaN
    EXPECT_DOUBLE_EQ(csv.columns[1].data[4], 50.0);
}

// --- get_clean_data (skip NA) ---

/// @brief Verify that get_clean_data with fail_on_na=false silently drops NaN entries.
TEST(CsvReaderTest, GetCleanData) {
    std::istringstream input("id,value\n1,10\n2,NA\n3,30\n4,\n5,50\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    auto clean = csv.get_clean_data("value", false);
    ASSERT_EQ(clean.size(), 3u);
    EXPECT_DOUBLE_EQ(clean[0], 10.0);
    EXPECT_DOUBLE_EQ(clean[1], 30.0);
    EXPECT_DOUBLE_EQ(clean[2], 50.0);
}

// --- get_clean_data (fail on NA) ---

/// @brief Verify that get_clean_data with fail_on_na=true throws when NaN values are present.
TEST(CsvReaderTest, FailOnNa) {
    std::istringstream input("id,value\n1,10\n2,NA\n3,30\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    EXPECT_THROW(csv.get_clean_data("value", true), std::runtime_error);
}

// --- TSV Auto-Detection ---

/// @brief Verify that tab-separated input is auto-detected without explicit configuration.
TEST(CsvReaderTest, TsvAutoDetect) {
    std::istringstream input("name\tvalue\nA\t10\nB\t20\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    ASSERT_EQ(csv.columns.size(), 2u);
    EXPECT_EQ(csv.columns[0].name, "name");
    EXPECT_EQ(csv.columns[1].name, "value");
    EXPECT_DOUBLE_EQ(csv.columns[1].data[0], 10.0);
}

// --- Windows CRLF ---

/// @brief Verify that Windows-style CRLF line endings are handled transparently.
TEST(CsvReaderTest, WindowsCRLF) {
    std::istringstream input("name,value\r\nA,10\r\nB,20\r\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    ASSERT_EQ(csv.columns.size(), 2u);
    ASSERT_EQ(csv.columns[1].data.size(), 2u);
    EXPECT_DOUBLE_EQ(csv.columns[1].data[0], 10.0);
    EXPECT_DOUBLE_EQ(csv.columns[1].data[1], 20.0);
}

// --- UTF-8 BOM ---

/// @brief Verify that a UTF-8 BOM prefix is stripped from the first column name.
TEST(CsvReaderTest, Utf8Bom) {
    std::string bom_csv = "\xEF\xBB\xBFname,value\nA,10\nB,20\n";
    std::istringstream input(bom_csv);
    CsvReader reader;
    auto csv = reader.read_stream(input);

    EXPECT_EQ(csv.columns[0].name, "name");  // BOM should be stripped
    EXPECT_EQ(csv.columns[1].name, "value");
}

// --- No Header (all numeric first row) ---

/// @brief Verify that an all-numeric first row triggers auto-generated column names (col1, col2, ...).
TEST(CsvReaderTest, NoHeaderAutoDetect) {
    std::istringstream input("1,10\n2,20\n3,30\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    // First row is all numeric -- auto-generated column names
    EXPECT_EQ(csv.columns[0].name, "col1");
    EXPECT_EQ(csv.columns[1].name, "col2");
    ASSERT_EQ(csv.columns[0].data.size(), 3u);
    EXPECT_DOUBLE_EQ(csv.columns[0].data[0], 1.0);
}

// --- Column Selection by Name ---

/// @brief Verify that get_column retrieves the correct column when addressed by name.
TEST(CsvReaderTest, ColByName) {
    std::istringstream input("x,y,z\n1,2,3\n4,5,6\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    const auto& col = csv.get_column("y");
    EXPECT_EQ(col.name, "y");
    EXPECT_DOUBLE_EQ(col.data[0], 2.0);
}

// --- Column Selection by Number ---

/// @brief Verify that get_column retrieves the correct column when addressed by 1-based index.
TEST(CsvReaderTest, ColByNumber) {
    std::istringstream input("x,y,z\n1,2,3\n4,5,6\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    const auto& col = csv.get_column("2");
    EXPECT_EQ(col.name, "y");
    EXPECT_DOUBLE_EQ(col.data[0], 2.0);
}

// --- Column Not Found ---

/// @brief Verify that get_column throws when the column name or index does not exist.
TEST(CsvReaderTest, ColNotFound) {
    std::istringstream input("x,y\n1,2\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    EXPECT_THROW(csv.get_column("z"), std::runtime_error);
    EXPECT_THROW(csv.get_column("5"), std::runtime_error);
}

// --- Empty Input ---

/// @brief Verify that reading an empty stream throws a runtime error.
TEST(CsvReaderTest, EmptyInput) {
    std::istringstream input("");
    CsvReader reader;

    EXPECT_THROW(reader.read_stream(input), std::runtime_error);
}

// --- Trailing Comma (empty final field) ---

/// @brief Verify that a trailing comma produces a NaN entry for the empty final field.
TEST(CsvReaderTest, TrailingComma) {
    std::istringstream input("a,b,c\n1,2,\n4,5,6\n");
    CsvReader reader;
    auto csv = reader.read_stream(input);

    ASSERT_EQ(csv.columns.size(), 3u);
    EXPECT_TRUE(std::isnan(csv.columns[2].data[0]));  // empty field becomes NaN
    EXPECT_DOUBLE_EQ(csv.columns[2].data[1], 6.0);
}
