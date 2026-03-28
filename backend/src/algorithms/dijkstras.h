#ifndef DIJKSTRAS_H
#define DIJKSTRAS_H
#include <string>
#include <map>
#include <unordered_map>
#include <vector>


std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(const std::map<std::pair<std::string, std::string>, int>& routes);

// dijkstra runs off of adjacency graph. Returns a vector of node indices for now.
std::pair<int, std::vector<std::string>> dijkstras(const std::string& source, const std::string& destination, std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes);
#endif