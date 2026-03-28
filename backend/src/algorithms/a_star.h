//
// Created by faken on 3/27/2026.
//

#ifndef AEROROUTE_A_STAR_H
#define AEROROUTE_A_STAR_H

#include <filesystem>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>

using AirportCoordinates = std::unordered_map<std::string, std::pair<double, double>>;

int heuristic(const std::string& x, const std::string& destination, const AirportCoordinates& coords);

AirportCoordinates load_airport_coordinates(const std::filesystem::path& csv_path);

std::pair<int, std::vector<std::string>> a_star(
    const std::string& source,
    const std::string& destination,
    std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes,
    const AirportCoordinates& coords
);


#endif //AEROROUTE_A_STAR_H
