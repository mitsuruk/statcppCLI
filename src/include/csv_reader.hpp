/**
 * @file csv_reader.hpp
 * @brief CSV file reader and data container for the statcpp CLI.
 *
 * Provides the CsvData structure for storing columnar numeric data and
 * the CsvReader class for parsing CSV/TSV files, standard input, and
 * row-oriented streams into CsvData.  Handles BOM stripping, automatic
 * delimiter detection, quoted fields, header auto-detection, and
 * configurable NA value recognition.
 */
#pragma once

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <statcpp/data_wrangling.hpp>

namespace statcpp_cli {

namespace fs = std::filesystem;

/**
 * @struct CsvData
 * @brief Container for columnar numeric data parsed from a CSV source.
 *
 * Each column stores a name and a vector of @c double values.
 * Non-numeric or missing entries are represented as @c NaN.
 */
struct CsvData {
    /**
     * @struct Column
     * @brief A single named column of numeric data.
     */
    struct Column {
        std::string name;              ///< Column header name (or auto-generated "col1", "col2", ...).
        std::vector<double> data;      ///< Numeric values; NaN indicates a missing value.
    };

    std::vector<Column> columns;       ///< All columns contained in this data set.

    /**
     * @brief Look up a column by name or 1-based index.
     *
     * If @p name_or_index is a valid integer string in the range
     * [1, columns.size()], the corresponding column is returned.
     * Otherwise, a name-based lookup is performed.
     *
     * @param name_or_index Column name or 1-based numeric index as a string.
     * @return A const reference to the matching Column.
     * @throws std::runtime_error If no column matches the given identifier.
     */
    const Column& get_column(const std::string& name_or_index) const {
        // Try as 1-based index
        try {
            std::size_t pos;
            int idx = std::stoi(name_or_index, &pos);
            if (pos == name_or_index.size() && idx >= 1 &&
                static_cast<std::size_t>(idx) <= columns.size()) {
                return columns[static_cast<std::size_t>(idx - 1)];
            }
        } catch (...) {}

        // Try as column name
        for (const auto& col : columns) {
            if (col.name == name_or_index) {
                return col;
            }
        }

        throw std::runtime_error("Column not found: " + name_or_index);
    }

    /**
     * @brief Retrieve a column's data with NaN values handled.
     *
     * When @p fail_on_na is @c true, the presence of any NaN value causes
     * an exception.  When @c false, NaN entries are silently dropped and
     * the remaining values are returned.
     *
     * @param col        Column name or 1-based index.
     * @param fail_on_na If @c true, throw on any missing value; otherwise drop NaNs.
     * @return A vector of clean (non-NaN) doubles.
     * @throws std::runtime_error If the column is not found or if @p fail_on_na
     *         is @c true and a NaN is encountered.
     */
    std::vector<double> get_clean_data(const std::string& col, bool fail_on_na) const {
        const auto& column = get_column(col);
        if (fail_on_na) {
            for (std::size_t i = 0; i < column.data.size(); ++i) {
                if (std::isnan(column.data[i])) {
                    throw std::runtime_error(
                        "Missing value found in column '" + column.name +
                        "' at row " + std::to_string(i + 1) +
                        " (use --skip_na or remove missing values)");
                }
            }
            return column.data;
        }
        return statcpp::dropna(column.data);
    }

    /**
     * @brief Split a comma-separated column specification string.
     *
     * For example, @c "col1,col2" is split into @c {"col1","col2"}.
     * Empty tokens between consecutive commas are silently discarded.
     *
     * @param col_spec Comma-separated list of column names or indices.
     * @return A vector of individual column identifiers.
     */
    static std::vector<std::string> split_col_spec(const std::string& col_spec) {
        std::vector<std::string> cols;
        std::istringstream ss(col_spec);
        std::string token;
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) cols.push_back(token);
        }
        return cols;
    }

    /**
     * @brief Retrieve paired data from two columns with row-wise NaN removal.
     *
     * Rows where either column contains NaN are removed from both output
     * vectors, keeping the pairing intact.  When @p fail_on_na is @c true,
     * an exception is thrown instead of dropping the row.
     *
     * @param col1       Name or 1-based index of the first column.
     * @param col2       Name or 1-based index of the second column.
     * @param fail_on_na If @c true, throw on any missing value; otherwise drop the row.
     * @return A pair of vectors (first column data, second column data) with matched rows.
     * @throws std::runtime_error If a column is not found or if @p fail_on_na
     *         is @c true and a NaN is encountered.
     */
    std::pair<std::vector<double>, std::vector<double>>
    get_paired_clean_data(const std::string& col1, const std::string& col2,
                          bool fail_on_na) const {
        const auto& c1 = get_column(col1);
        const auto& c2 = get_column(col2);
        std::size_t n = std::min(c1.data.size(), c2.data.size());

        std::vector<double> d1, d2;
        d1.reserve(n);
        d2.reserve(n);

        for (std::size_t i = 0; i < n; ++i) {
            bool na1 = std::isnan(c1.data[i]);
            bool na2 = std::isnan(c2.data[i]);
            if (na1 || na2) {
                if (fail_on_na) {
                    throw std::runtime_error(
                        "Missing value at row " + std::to_string(i + 1));
                }
                continue;
            }
            d1.push_back(c1.data[i]);
            d2.push_back(c2.data[i]);
        }
        return {d1, d2};
    }
};

/**
 * @class CsvReader
 * @brief Reads CSV/TSV data from files or streams into a CsvData object.
 *
 * Features include:
 * - Automatic delimiter detection (tab, comma, space) or explicit override.
 * - Automatic header-row detection (skipped when every field is numeric).
 * - UTF-8 BOM stripping.
 * - RFC 4180 quoted-field handling (including escaped double-quotes).
 * - Configurable set of strings treated as missing / NA values.
 * - A "row mode" that reads whitespace/comma-separated values into a
 *   single column.
 */
class CsvReader {
public:
    char delimiter = '\0';             ///< Field delimiter override; @c '\\0' means auto-detect.
    bool has_header = true;            ///< Whether the first row should be treated as a header.
    std::vector<std::string> na_values = {"NA", "NaN", "nan", "N/A", "n/a"}; ///< Strings recognised as missing values.

    /**
     * @brief Replace the set of NA indicator strings.
     *
     * The provided string is split on commas to produce individual tokens.
     * For example, @c "NA,NaN,." sets three NA representations.
     *
     * @param na_str Comma-separated list of strings to treat as missing values.
     */
    void set_na_string(const std::string& na_str) {
        na_values.clear();
        std::istringstream ss(na_str);
        std::string token;
        while (std::getline(ss, token, ',')) {
            na_values.push_back(token);
        }
    }

    /**
     * @brief Read a CSV file from disk.
     *
     * @param filepath Path to the CSV file.
     * @return A CsvData object containing the parsed columns.
     * @throws std::runtime_error If the file cannot be opened.
     */
    CsvData read_file(const std::string& filepath) const {
        std::ifstream file{fs::path(filepath)};
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open: " + filepath);
        }
        return read_stream(file);
    }

    /**
     * @brief Read CSV data from standard input.
     *
     * @return A CsvData object containing the parsed columns.
     * @throws std::runtime_error If the input is empty.
     */
    CsvData read_stdin() const {
        return read_stream(std::cin);
    }

    /**
     * @brief Read row-oriented data from an input stream (row mode).
     *
     * Each line is tokenised on commas, spaces, and tabs.  All values are
     * collected into a single column named @c "col1".  Non-numeric tokens
     * and configured NA strings are stored as NaN.
     *
     * @param input The input stream to read from.
     * @return A CsvData object with a single column containing all values.
     * @throws std::runtime_error If the resulting column is empty.
     */
    CsvData read_row_stream(std::istream& input) const {
        CsvData result;
        result.columns.resize(1);
        result.columns[0].name = "col1";

        std::string line;
        while (std::getline(input, line)) {
            strip_cr(line);
            if (line.empty()) continue;

            // Split by comma and space (both are separators)
            std::string token;
            for (std::size_t i = 0; i < line.size(); ++i) {
                char c = line[i];
                if (c == ',' || c == ' ' || c == '\t') {
                    if (!token.empty()) {
                        add_row_value(result, token);
                        token.clear();
                    }
                } else {
                    token += c;
                }
            }
            if (!token.empty()) {
                add_row_value(result, token);
            }
        }

        if (result.columns[0].data.empty()) {
            throw std::runtime_error("Empty input");
        }
        return result;
    }

    /**
     * @brief Read row-oriented data from standard input (row mode).
     *
     * Convenience wrapper around read_row_stream() using @c std::cin.
     *
     * @return A CsvData object with a single column containing all values.
     * @throws std::runtime_error If the resulting column is empty.
     */
    CsvData read_row_stdin() const {
        return read_row_stream(std::cin);
    }

    /**
     * @brief Read columnar CSV data from a generic input stream.
     *
     * Performs BOM stripping, delimiter detection (or uses the configured
     * override), header auto-detection, and parses all rows into the
     * returned CsvData.  Fields that cannot be converted to @c double
     * are stored as NaN.
     *
     * @param input The input stream to read from.
     * @return A CsvData object containing the parsed columns.
     * @throws std::runtime_error If the input is empty.
     */
    CsvData read_stream(std::istream& input) const {
        skip_bom(input);

        std::string first_line;
        if (!std::getline(input, first_line)) {
            throw std::runtime_error("Empty input");
        }
        strip_cr(first_line);

        char delim = (delimiter != '\0') ? delimiter : detect_delimiter(first_line);
        auto header_fields = split_fields(first_line, delim);
        std::size_t ncols = header_fields.size();

        CsvData result;
        result.columns.resize(ncols);

        bool treat_as_header = has_header;
        if (has_header) {
            // Auto-detect: if all fields parse as numbers, treat as data
            bool all_numeric = true;
            for (const auto& f : header_fields) {
                if (f.empty()) continue;
                try {
                    std::size_t pos;
                    (void)std::stod(f, &pos);
                    if (pos != f.size()) all_numeric = false;
                } catch (...) {
                    all_numeric = false;
                }
                if (!all_numeric) break;
            }
            if (all_numeric && !header_fields.empty()) {
                treat_as_header = false;
            }
        }

        if (treat_as_header) {
            for (std::size_t i = 0; i < ncols; ++i) {
                result.columns[i].name = header_fields[i];
            }
        } else {
            for (std::size_t i = 0; i < ncols; ++i) {
                result.columns[i].name = "col" + std::to_string(i + 1);
            }
            add_row(result, header_fields, ncols);
        }

        std::string line;
        while (std::getline(input, line)) {
            strip_cr(line);
            if (line.empty()) continue;
            auto fields = split_fields(line, delim);
            add_row(result, fields, ncols);
        }

        return result;
    }

private:
    /**
     * @brief Skip a UTF-8 BOM (byte order mark) at the start of the stream.
     *
     * If the first three bytes are @c 0xEF 0xBB 0xBF they are consumed;
     * otherwise the stream position is rewound.
     *
     * @param is The input stream to check.
     */
    static void skip_bom(std::istream& is) {
        if (is.peek() == 0xEF) {
            char bom[3];
            is.read(bom, 3);
            if (!(bom[0] == '\xEF' && bom[1] == '\xBB' && bom[2] == '\xBF')) {
                is.seekg(0);
            }
        }
    }

    /**
     * @brief Strip a trailing carriage-return character from a string.
     *
     * Used to normalise Windows-style line endings (@c \\r\\n) after
     * @c std::getline has already consumed the @c \\n.
     *
     * @param s The string to modify in place.
     */
    static void strip_cr(std::string& s) {
        if (!s.empty() && s.back() == '\r') {
            s.pop_back();
        }
    }

    /**
     * @brief Auto-detect the field delimiter from a line of text.
     *
     * Detection priority: tab > comma > space.  Falls back to comma if
     * none of the candidates are found.
     *
     * @param line A single line of CSV text (typically the first line).
     * @return The detected delimiter character.
     */
    static char detect_delimiter(const std::string& line) {
        if (line.find('\t') != std::string::npos) return '\t';
        if (line.find(',') != std::string::npos) return ',';
        if (line.find(' ') != std::string::npos) return ' ';
        return ',';
    }

    /**
     * @brief Split a line into fields using the given delimiter.
     *
     * Supports RFC 4180 double-quote escaping: a field may be enclosed in
     * double quotes, and literal double-quote characters inside a quoted
     * field are represented as @c "".
     *
     * @param line  The text line to split.
     * @param delim The delimiter character to split on.
     * @return A vector of field strings.
     */
    static std::vector<std::string> split_fields(const std::string& line, char delim) {
        std::vector<std::string> fields;
        std::string field;
        bool in_quotes = false;

        for (std::size_t i = 0; i < line.size(); ++i) {
            char c = line[i];
            if (in_quotes) {
                if (c == '"') {
                    if (i + 1 < line.size() && line[i + 1] == '"') {
                        field += '"';
                        ++i;
                    } else {
                        in_quotes = false;
                    }
                } else {
                    field += c;
                }
            } else {
                if (c == '"') {
                    in_quotes = true;
                } else if (c == delim) {
                    fields.push_back(field);
                    field.clear();
                } else {
                    field += c;
                }
            }
        }
        fields.push_back(field);
        return fields;
    }

    /**
     * @brief Parse a single token and append it to the first column of @p result.
     *
     * The token is converted to @c double.  If conversion fails or the
     * token matches a configured NA string, NaN is appended instead.
     * Used exclusively in row mode.
     *
     * @param result The CsvData whose first column receives the value.
     * @param token  The string token to parse.
     */
    void add_row_value(CsvData& result, const std::string& token) const {
        if (is_na_value(token)) {
            result.columns[0].data.push_back(
                std::numeric_limits<double>::quiet_NaN());
        } else {
            try {
                std::size_t pos;
                double val = std::stod(token, &pos);
                if (pos != token.size()) {
                    result.columns[0].data.push_back(
                        std::numeric_limits<double>::quiet_NaN());
                } else {
                    result.columns[0].data.push_back(val);
                }
            } catch (...) {
                result.columns[0].data.push_back(
                    std::numeric_limits<double>::quiet_NaN());
            }
        }
    }

    /**
     * @brief Test whether a string should be treated as a missing value.
     *
     * Empty strings are always considered missing.  Otherwise the string is
     * compared against every entry in the na_values list.
     *
     * @param s The string to test.
     * @return @c true if @p s represents a missing value; @c false otherwise.
     */
    bool is_na_value(const std::string& s) const {
        if (s.empty()) return true;
        return std::find(na_values.begin(), na_values.end(), s) != na_values.end();
    }

    /**
     * @brief Parse a row of fields and append values to the corresponding columns.
     *
     * Each field is converted to @c double.  Fields that are missing (beyond
     * the number of provided fields), match an NA string, or fail numeric
     * conversion are stored as NaN.
     *
     * @param result The CsvData to append values to.
     * @param fields The split field strings for this row.
     * @param ncols  The expected number of columns.
     */
    void add_row(CsvData& result, const std::vector<std::string>& fields,
                 std::size_t ncols) const {
        for (std::size_t i = 0; i < ncols; ++i) {
            if (i >= fields.size() || is_na_value(fields[i])) {
                result.columns[i].data.push_back(
                    std::numeric_limits<double>::quiet_NaN());
            } else {
                try {
                    std::size_t pos;
                    double val = std::stod(fields[i], &pos);
                    if (pos != fields[i].size()) {
                        // Not fully numeric -- treated as NaN
                        result.columns[i].data.push_back(
                            std::numeric_limits<double>::quiet_NaN());
                    } else {
                        result.columns[i].data.push_back(val);
                    }
                } catch (...) {
                    result.columns[i].data.push_back(
                        std::numeric_limits<double>::quiet_NaN());
                }
            }
        }
    }
};

}  // namespace statcpp_cli
