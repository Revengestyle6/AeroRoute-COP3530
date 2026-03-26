#ifndef DIJKSTRAS_H
#define DIJKSTRAS_H
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
//maybe create a class for flight sim that holds all the routes
//dont load it everytime

std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(std::map<std::pair<std::string, std::string>, double> routes);

// dijkstra runs off of adjacency graph. Returns a vector of node indices for now.
std::vector<int> dijkstras(const std::string& source, const std::string& destination, std::map<std::pair<std::string, std::string>, double> routes);
#endif