#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>

#include "a_star.h"
#include "route_data.h"
#include "dijkstras.h"


namespace fs = std::filesystem;

int main(int argc, char **argv) {
    try {
        fs::path base = fs::current_path();
        fs::path csv_path = base / "flight_data.csv";
        if (!fs::exists(csv_path)) {
            std::cerr << "CSV file not found at: " << csv_path << "\n";
            return 1;
        }

        auto origins = findoriginnames(csv_path);
        // for (const auto &p : origins) {
        //     std::cout << p.first << " -> '" << p.second << "'\n";
        // }

        auto routes = findallroutes(csv_path);
        // show a few sample routes with distances
        // for (const auto &kv : routes) {
        //     const auto &pair = kv.first;
        //     double dist = kv.second;
        //     std::cout << pair.first << " -> " << pair.second << " : " << std::fixed << std::setprecision(2) << dist << "\n";
        // }

        // Test input
        // std::string source = argv[1];
        // std::string destination = argv[2];
        // std::cout << "Source: " << source << ", Destination: " << destination << std::endl;

        auto flight_graph = adjacency_list(routes);
        
        auto output = dijkstras("MIA", "LAX", flight_graph);
        std::cout << output.first << std::endl;
        for (const auto& airport : output.second) {
            std::cout << airport << " ";
        }

        auto output_a_star = a_star("MIA", "LAX", flight_graph);
        std::cout << output_a_star.first << std::endl;
        for (const auto& airport : output_a_star.second) {
            std::cout << airport << " ";
        }
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
    std::cout << "Processing completed successfully.\n";
    return 0;
}
