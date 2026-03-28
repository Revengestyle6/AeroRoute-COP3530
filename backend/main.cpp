#include <chrono>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "algorithms/a_star.h"
#include "algorithms/dijkstras.h"
#include "data/csv_loader.h"
#include "data/route_data.h"

namespace fs = std::filesystem;

// scope for creating JSON text
namespace {


// JSON text to return to Node server (which is then parsed to create JSON object)
std::string escape_json(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());

    for (char ch : value) {
        switch (ch) {
            case '\\':
                escaped += "\\\\";
                break;
            case '"':
                escaped += "\\\"";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }

    return escaped;
}

void print_json_array(const std::vector<std::string>& values) {
    std::cout << "[";

    for (size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            std::cout << ",";
        }

        std::cout << "\"" << escape_json(values[index]) << "\"";
    }

    std::cout << "]";
}

void print_algorithm_result(const std::pair<int, std::vector<std::string>>& result,
                            double time_ms) {
    std::cout << "{";
    std::cout << "\"path\":";
    print_json_array(result.second);
    std::cout << ",\"distance\":" << result.first;
    std::cout << ",\"timeMs\":" << std::fixed << std::setprecision(3) << time_ms;
    std::cout << "}";
}
} 

int main(int argc, char **argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <source> <destination>\n";
        return 1;
    }

    const std::string source = upper(trim(argv[1]));
    const std::string destination = upper(trim(argv[2]));

    try {
        fs::path base = fs::current_path();
        fs::path csv_path = base / "flight_data.csv";
        if (!fs::exists(csv_path)) {
            std::cerr << "CSV file not found at: " << csv_path << "\n";
            return 1;
        }

        auto routes = findallroutes(csv_path);
        auto flight_graph = adjacency_list(routes);

        const auto dijkstra_start = std::chrono::steady_clock::now();
        const auto dijkstra_result = dijkstras(source, destination, flight_graph);
        const auto dijkstra_end = std::chrono::steady_clock::now();

        const auto astar_start = std::chrono::steady_clock::now();
        const auto astar_result = a_star(source, destination, flight_graph);
        const auto astar_end = std::chrono::steady_clock::now();

        const double dijkstra_time_ms =
            std::chrono::duration<double, std::milli>(dijkstra_end - dijkstra_start).count();
        const double astar_time_ms =
            std::chrono::duration<double, std::milli>(astar_end - astar_start).count();

        std::cout << "{";
        std::cout << "\"dijkstra\":";
        print_algorithm_result(dijkstra_result, dijkstra_time_ms);
        std::cout << ",\"astar\":";
        print_algorithm_result(astar_result, astar_time_ms);
        std::cout << "}\n";
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
}
