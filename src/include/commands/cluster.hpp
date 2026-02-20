/**
 * @file cluster.hpp
 * @brief Clustering command handler for the statcpp CLI.
 *
 * Provides the entry point for clustering-related subcommands, including
 * k-means clustering, agglomerative (single-linkage) hierarchical clustering,
 * and silhouette score evaluation. Each subcommand reads multi-column CSV
 * data, constructs a feature matrix, and delegates to the corresponding
 * statcpp library routines.
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
 * @brief Execute a clustering subcommand.
 *
 * Dispatches to one of the supported clustering operations based on
 * @p cmd.command:
 *   - @c "kmeans" -- Runs k-means clustering (k=3) on the selected columns,
 *     reporting inertia, iteration count, centroids, and cluster sizes.
 *   - @c "hierarchical" -- Performs single-linkage agglomerative hierarchical
 *     clustering and prints the resulting dendrogram (merge steps).
 *   - @c "silhouette" -- Computes the silhouette score for a k-means (k=3)
 *     partition and provides a qualitative interpretation of the result.
 *
 * All subcommands require at least two columns (features) specified via the
 * @c --col flag. Column data is assembled into an n-by-p matrix where each
 * row is an observation and each column is a feature.
 *
 * @param cmd  Parsed command containing the subcommand name in
 *             @c cmd.command (e.g. "kmeans", "hierarchical", "silhouette").
 * @param csv  CSV data source from which feature columns are extracted.
 * @param fmt  Output formatter that controls text vs. JSON rendering.
 * @return 0 on successful execution.
 * @throws std::runtime_error If fewer than two columns are specified for any
 *         subcommand, or if @p cmd.command is not a recognised clustering
 *         subcommand.
 */
inline int run_cluster(const ParsedCommand& cmd, CsvData& csv, OutputFormatter& fmt) {
    auto cols = CsvData::split_col_spec(FLAGS_col);

    fmt.set_command("cluster." + cmd.command);

    // Build data matrix from columns (each column = a feature)
    auto build_data_matrix = [&]() {
        std::vector<std::vector<double>> col_data;
        for (const auto& c : cols) {
            col_data.push_back(csv.get_clean_data(c, FLAGS_fail_na));
        }
        std::size_t n = col_data[0].size();
        std::size_t p = cols.size();
        std::vector<std::vector<double>> data(n, std::vector<double>(p));
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < p; ++j) {
                data[i][j] = col_data[j][i];
            }
        }
        return data;
    };

    if (cmd.command == "kmeans") {
        if (cols.size() < 2) {
            throw std::runtime_error("kmeans requires 2+ columns (features)");
        }
        auto data = build_data_matrix();
        // Default k=3
        std::size_t k = 3;
        auto r = statcpp::kmeans(data, k);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"n", data.size()}, {"k", k}});
        fmt.print({
            {"Inertia",     r.inertia},
            {"Iterations",  static_cast<double>(r.n_iter)},
            {"K",           static_cast<double>(k)},
        });

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "\n  Centroids:" << std::endl;
            std::cout << "  " << std::left << std::setw(10) << "Cluster";
            for (const auto& c : cols) {
                std::cout << std::setw(12) << c;
            }
            std::cout << std::endl;
            for (std::size_t i = 0; i < r.centroids.size(); ++i) {
                std::cout << "  " << std::left << std::setw(10) << (i + 1);
                for (double v : r.centroids[i]) {
                    std::cout << std::setw(12) << v;
                }
                std::cout << std::endl;
            }

            // Cluster sizes
            std::vector<std::size_t> sizes(k, 0);
            for (auto l : r.labels) sizes[l]++;
            std::cout << "\n  Cluster sizes:";
            for (std::size_t i = 0; i < k; ++i) {
                std::cout << " " << (i + 1) << "=" << sizes[i];
            }
            std::cout << std::endl;
        }

    } else if (cmd.command == "hierarchical") {
        if (cols.size() < 2) {
            throw std::runtime_error("hierarchical requires 2+ columns (features)");
        }
        auto data = build_data_matrix();
        auto dendro = statcpp::hierarchical_clustering(data);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"n", data.size()}, {"linkage", "single"}});

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            std::cout << "  Dendrogram:" << std::endl;
            std::cout << "  " << std::left << std::setw(8) << "Step"
                      << std::setw(8) << "Left"
                      << std::setw(8) << "Right"
                      << std::setw(12) << "Distance"
                      << "Count" << std::endl;
            for (std::size_t i = 0; i < dendro.size(); ++i) {
                std::cout << "  " << std::left << std::setw(8) << (i + 1)
                          << std::setw(8) << dendro[i].left
                          << std::setw(8) << dendro[i].right
                          << std::setw(12) << dendro[i].distance
                          << dendro[i].count << std::endl;
            }
        } else {
            for (std::size_t i = 0; i < dendro.size(); ++i) {
                fmt.print("Step " + std::to_string(i + 1) + " dist", dendro[i].distance);
            }
        }

    } else if (cmd.command == "silhouette") {
        if (cols.size() < 2) {
            throw std::runtime_error("silhouette requires 2+ columns (features)");
        }
        auto data = build_data_matrix();
        // Default k=3
        std::size_t k = 3;
        auto km = statcpp::kmeans(data, k);
        double score = statcpp::silhouette_score(data, km.labels);

        nlohmann::json col_names = cols;
        fmt.set_input_info({{"columns", col_names}, {"n", data.size()}, {"k", k}});
        fmt.print("Silhouette score", score);

        if (fmt.get_mode() == OutputFormatter::Mode::Text) {
            if (score > 0.7) {
                std::cout << "  Interpretation: strong structure" << std::endl;
            } else if (score > 0.5) {
                std::cout << "  Interpretation: reasonable structure" << std::endl;
            } else if (score > 0.25) {
                std::cout << "  Interpretation: weak structure" << std::endl;
            } else {
                std::cout << "  Interpretation: no substantial structure" << std::endl;
            }
        }

    } else {
        throw std::runtime_error("Unknown cluster command: " + cmd.command +
            "\nAvailable: kmeans, hierarchical, silhouette");
    }

    fmt.flush();
    return 0;
}

}  // namespace commands
}  // namespace statcpp_cli
