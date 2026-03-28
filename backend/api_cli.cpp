#include <algorithm>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "a_star.h"
#include "csv_loader.h"
#include "dijkstras.h"
#include "route_data.h"

namespace fs = std::filesystem;

namespace {

std::string json_escape(const std::string &s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\b':
                out += "\\b";
                break;
            case '\f':
                out += "\\f";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                out += c;
                break;
        }
    }
    return out;
}

std::unordered_map<std::string, std::pair<double, double>> load_coords(const fs::path &csv_path) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csv_path, header, rows)) {
        throw std::runtime_error("Unable to read CSV: " + csv_path.string());
    }

    auto idx_iata = column_index(header, "IATA");
    auto idx_lat = column_index(header, "LATITUDE");
    auto idx_lon = column_index(header, "LONGITUDE");
    if (!idx_iata || !idx_lat || !idx_lon) {
        throw std::runtime_error("Missing required columns in location CSV");
    }

    std::unordered_map<std::string, std::pair<double, double>> coords;
    for (const auto &row : rows) {
        std::string iata = (*idx_iata < row.size()) ? upper(trim(row[*idx_iata])) : "";
        std::string lat = (*idx_lat < row.size()) ? trim(row[*idx_lat]) : "";
        std::string lon = (*idx_lon < row.size()) ? trim(row[*idx_lon]) : "";
        if (iata.empty() || lat.empty() || lon.empty()) {
            continue;
        }

        try {
            coords[iata] = {std::stod(lat), std::stod(lon)};
        } catch (...) {
            continue;
        }
    }
    return coords;
}

void print_airports_json(const std::map<std::pair<std::string, std::string>, int> &routes) {
    std::set<std::string> airports;
    for (const auto &entry : routes) {
        airports.insert(entry.first.first);
        airports.insert(entry.first.second);
    }

    std::cout << "{\"airports\":[";
    bool first = true;
    for (const auto &airport : airports) {
        if (!first) {
            std::cout << ",";
        }
        first = false;
        std::cout << "\"" << json_escape(airport) << "\"";
    }
    std::cout << "]}";
}

void print_route_json(
    const std::string &algorithm,
    const std::string &source,
    const std::string &destination,
    const std::pair<int, std::vector<std::string>> &result,
    long long elapsed_us,
    const std::unordered_map<std::string, std::pair<double, double>> &coords) {
    std::cout << "{";
    std::cout << "\"algorithm\":\"" << json_escape(algorithm) << "\",";
    std::cout << "\"source\":\"" << json_escape(source) << "\",";
    std::cout << "\"destination\":\"" << json_escape(destination) << "\",";
    std::cout << "\"distance\":" << result.first << ",";
    std::cout << "\"elapsed_us\":" << elapsed_us << ",";

    std::cout << "\"path\":[";
    for (size_t i = 0; i < result.second.size(); ++i) {
        if (i > 0) {
            std::cout << ",";
        }
        std::cout << "\"" << json_escape(result.second[i]) << "\"";
    }
    std::cout << "],";

    std::cout << "\"coordinates\":[";
    bool first = true;
    for (const auto &airport : result.second) {
        auto it = coords.find(airport);
        if (it == coords.end()) {
            continue;
        }

        if (!first) {
            std::cout << ",";
        }
        first = false;

        std::cout << "{";
        std::cout << "\"iata\":\"" << json_escape(airport) << "\",";
        std::cout << "\"lat\":" << it->second.first << ",";
        std::cout << "\"lon\":" << it->second.second;
        std::cout << "}";
    }
    std::cout << "]";
    std::cout << "}";
}

}  // namespace

int main(int argc, char **argv) {
    try {
        fs::path base = fs::current_path();
        fs::path routes_csv = base / "flight_data.csv";
        fs::path coords_csv = base / "accurate_airport_locations.csv";

        if (!fs::exists(routes_csv)) {
            std::cerr << "CSV file not found at: " << routes_csv << "\n";
            return 1;
        }

        auto routes = findallroutes(routes_csv);

        if (argc < 2) {
            std::cerr << "Usage: aeroroute_api <airports|route> [algorithm source destination]\n";
            return 1;
        }

        std::string command = upper(trim(argv[1]));
        if (command == "AIRPORTS") {
            print_airports_json(routes);
            return 0;
        }

        if (command != "ROUTE") {
            std::cerr << "Unknown command: " << command << "\n";
            return 1;
        }

        if (argc < 5) {
            std::cerr << "Usage: aeroroute_api route <dijkstra|astar> <source> <destination>\n";
            return 1;
        }

        std::string algorithm = upper(trim(argv[2]));
        std::string source = upper(trim(argv[3]));
        std::string destination = upper(trim(argv[4]));

        auto graph = adjacency_list(routes);
        auto coords = load_coords(coords_csv);

        auto started = std::chrono::high_resolution_clock::now();
        std::pair<int, std::vector<std::string>> result;

        if (algorithm == "DIJKSTRA" || algorithm == "DIJKSTRAS") {
            result = dijkstras(source, destination, graph);
            algorithm = "dijkstra";
        } else if (algorithm == "A*" || algorithm == "ASTAR" || algorithm == "A_STAR") {
            result = a_star(source, destination, graph);
            algorithm = "astar";
        } else {
            std::cerr << "Unknown algorithm: " << argv[2] << "\n";
            return 1;
        }

        auto ended = std::chrono::high_resolution_clock::now();
        long long elapsed_us =
            std::chrono::duration_cast<std::chrono::microseconds>(ended - started).count();

        print_route_json(algorithm, source, destination, result, elapsed_us, coords);
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";
        return 2;
    }
}
