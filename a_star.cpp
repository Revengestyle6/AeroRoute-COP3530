#include "dijkstras.h"
#include "a_star.h"
#include <algorithm>
#include <iostream>
#include <utility>
#include <cmath>
#include "csv_loader.h"

int heuristic(const std::string &x, const std::string &destination, const std::unordered_map<std::string, std::pair<double,double>>& coords) {
    if (coords.find(x) == coords.end() || coords.find(destination) == coords.end()) return 0;

    double latOrigin = coords.at(x).first;
    double lonOrigin = coords.at(x).second;
    double latDestination = coords.at(x).first;
    double lonDestination = coords.at(x).second;

    //Haversine formula to find distance between two points
    double dlat = (latDestination - latOrigin) * 3.1415192 / 180.0;
    double dlon = (lonDestination - lonOrigin) * 3.1415192 / 180.0;

    double lat1 = latOrigin * 3.1415192 / 180.0;
    double lat2 = latDestination * 3.1415192 / 180.0;

    double h = sin(dlat/2)*sin(dlat/2) +
               cos(lat1)*cos(lat2) *
               sin(dlon/2)*sin(dlon/2);

    double c = 2 * asin(sqrt(h));
    return (int)(6371 * c);
}


std::pair<int, std::vector<std::string>> a_star(const std::string& source, const std::string& destination, std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes) {
    // initializing coordinate search
    std::filesystem::path base = std::filesystem::current_path();
    std::filesystem::path csv_path = base / "accurate_airport_locations.csv";

    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;

    if (!read_csv(csv_path, header, rows)) {
        std::cout << "Unable to read CSV: " << csv_path.string() << std::endl;
        throw std::runtime_error("Unable to read CSV: " + csv_path.string());
    }

    auto idx_city = column_index(header, "IATA");
    auto idx_lat = column_index(header, "LATITUDE");
    auto idx_long = column_index(header, "LONGITUDE");

    if (!idx_city || !idx_lat || !idx_long) {
        throw std::runtime_error("Missing columns");
    }

    // Build coordinate map ONCE
    std::unordered_map<std::string, std::pair<double,double>> coords;

    for (const auto &row : rows) {
        std::string airport = (*idx_city < row.size()) ? row[*idx_city] : "";
        std::string lat = (*idx_lat < row.size()) ? row[*idx_lat] : "";
        std::string lon = (*idx_long < row.size()) ? row[*idx_long] : "";

        if (!airport.empty() && !lat.empty() && !lon.empty()) {
            coords[airport] = {std::stod(lat), std::stod(lon)};
        }
    }


    std::vector<std::string> finished;
    std::vector<std::string> unfinished;
    std::unordered_map<std::string, int> distance;
    std::unordered_map<std::string, std::string> predecessor;

    //initialize finished list
    finished.emplace_back(source);
    distance[source] = 0;
    predecessor[source] = "N/A";
    //initialize unfinished list
    for (const auto& origin : routes) {
        std::cout << "initializing a_star" << std::endl;
        if (origin.first != source) {
            unfinished.emplace_back(origin.first);
            distance[origin.first] = INT_MAX;
            predecessor[origin.first] = "N/A";
        }
    }

    for (const auto& route : routes[source]) {
        distance[route.first] = route.second;
        predecessor[route.first] = source;
    }

    while (!std::empty(unfinished)) {
        std::cout << "running a_star" << std::endl;
        std::string smallest;
        int min = INT_MAX;

        for (const auto& x : unfinished) {
            int f = distance[x] + heuristic(x, destination, coords);
            if (f < min) {
                min = f;
                smallest = x;
            }
        }
        std::cout << smallest << std::endl;
        finished.emplace_back(smallest);
        unfinished.erase(std::remove(unfinished.begin(), unfinished.end(), smallest), unfinished.end());

        for (const auto& route : routes[smallest]) {
            if (distance[smallest] + route.second <= distance[route.first]) {
                distance[route.first] = distance[smallest] + route.second;
                predecessor[route.first] = smallest;
            }
        }
        if (smallest == destination) {
            break;
        }
    }
    int outputDistance = distance[destination];
    std::vector<std::string> path = {};
    std::string curr = destination;
    path.push_back(destination);
    while (predecessor[curr] != "N/A") {
        std::cout << "running pathfinding" << std::endl;
        std::cout << predecessor[curr] << std::endl;
        path.insert(path.begin(), predecessor[curr]);
        curr = predecessor[curr];
    }

    return std::make_pair(outputDistance, path);
}