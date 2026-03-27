#include <filesystem>
#include <iomanip>
#include <iostream>
#include <vector>
#include <string>

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
        std::string source;
        std::string destination;

        if (argc >= 3) {
            source = argv[1];
            destination = argv[2];
        } else {
            // Interactive prompt when arguments are missing
            std::cout << "Enter source airport (e.g. ATL): ";
            if (!std::getline(std::cin, source) || source.empty()) {
                std::cerr << "No source provided. Exiting.\n";
                return 1;
            }
            std::cout << "Enter destination airport (e.g. LAX): ";
            if (!std::getline(std::cin, destination) || destination.empty()) {
                std::cerr << "No destination provided. Exiting.\n";
                return 1;
            }
        }

        std::cout << "Source: " << source << ", Destination: " << destination << std::endl;

        auto flight_graph = adjacency_list(routes);
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
    std::cout << "Processing completed successfully.\n";
}
