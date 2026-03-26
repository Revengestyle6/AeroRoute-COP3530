#include <filesystem>
#include <iomanip>
#include <iostream>

#include "route_data.h"

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
        std::cout << "Found " << origins.size() << " origin airports (showing up to 20):\n";
        size_t shown = 0;
        for (const auto &p : origins) {
            std::cout << p.first << " -> '" << p.second << "'\n";
            if (++shown >= 20) break;
        }

        auto routes = findallroutes(csv_path);
        std::cout << "Total unique route pairs stored (both directions): " << routes.size() << "\n";
        // show a few sample routes with distances
        for (const auto &kv : routes) {
            const auto &pair = kv.first;
            double dist = kv.second;
            std::cout << pair.first << " -> " << pair.second << " : " << std::fixed << std::setprecision(2) << dist << "\n";
        }
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
}
