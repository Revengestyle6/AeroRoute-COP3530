//
// Created by faken on 3/27/2026.
//

#ifndef AEROROUTE_A_STAR_H
#define AEROROUTE_A_STAR_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>


// for a* that modifies weights of dijkstra's algorithm
int heuristic(const std::string& x, const std::string& destination, const std::unordered_map<std::string, std::pair<double,double>>& coords);
// dijkstra runs off of adjacency graph. Returns a vector of node indices for now.
std::pair<int, std::vector<std::string>> a_star(const std::string& source, const std::string& destination, std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes);


#endif //AEROROUTE_A_STAR_H