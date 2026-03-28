#include "dijkstras.h"
#include "a_star.h"
#include <algorithm>
#include <iostream>
#include <utility>
#include <cmath>
#include "csv_loader.h"
#include <queue>

// used for solving g var (estimated distance from currNode -> destination) in A*
int heuristic(const std::string &x, const std::string &destination, const std::unordered_map<std::string, std::pair<double,double>>& coords) {
    if (coords.find(x) == coords.end() || coords.find(destination) == coords.end()) return 0;

    // source
    double latOrigin = coords.at(x).first;
    double lonOrigin = coords.at(x).second;

    // target
    double latDestination = coords.at(destination).first;
    double lonDestination = coords.at(destination).second;

    //Haversine formula to find distance between two points (used for calculating with curvature of Earth)
    double dlat = (latDestination - latOrigin) * 3.1415192 / 180.0;
    double dlon = (lonDestination - lonOrigin) * 3.1415192 / 180.0;

    double lat1 = latOrigin * 3.1415192 / 180.0;
    double lat2 = latDestination * 3.1415192 / 180.0;

    double h = sin(dlat/2)*sin(dlat/2) +
               cos(lat1)*cos(lat2) *
               sin(dlon/2)*sin(dlon/2);

    double c = 2 * asin(sqrt(h));
    return (int)(3959 * c);
}


std::pair<int, std::vector<std::string>> a_star(const std::string& source, const std::string& destination, std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes) {
 
    std::cout << "Starting a_star" << std::endl;

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

    // initialize priority queue (min heap behavior)
    std::priority_queue<std::pair<int, std::string>, std::vector<std::pair<int, std::string>>, std::greater<>> pq;

    // std::vector<std::string> finished;
    // std::vector<std::string> unfinished;
    std::unordered_map<std::string, int> distance;
    std::unordered_map<std::string, std::string> predecessor;

    for (const auto& origin : routes) {
        if (origin.first != source) {
            distance[origin.first] = INT_MAX;
            predecessor[origin.first] = "N/A";
        }
    }

    // add source to priority queue w/ distance 0
    pq.emplace(0, source);
    distance[source] = 0;
    predecessor[source] = "N/A";

    while (!pq.empty()) {
        int min = INT_MAX;

        std::pair<int, std::string> current = pq.top();
        int currentF = current.first;
        std::string currentAirport = current.second;
        pq.pop();

        if (currentF > distance[currentAirport] + heuristic(currentAirport, destination, coords)) {
            continue;
        }

        if (currentAirport == destination) {
            break;
        }

        // relaxation: loop through every neighbor of current
        // routes = [string, int]
        for (const auto& route : routes[currentAirport]) {
            // if distance of current + distance to neighbor less than distance in array -> replace distance 
            if (distance[currentAirport] + route.second < distance[route.first]){
                distance[route.first] = distance[currentAirport] + route.second;
                predecessor[route.first] = currentAirport;

                // compute heuristic and add to pq
                int f = distance[route.first] + heuristic(route.first, destination, coords);
                pq.emplace(f, route.first);

            }
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
