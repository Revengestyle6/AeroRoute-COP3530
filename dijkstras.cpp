#include "dijkstras.h"

#include <utility>

std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(std::map<std::pair<std::string, std::string>, double> routes){
    std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adj_list;
    for (auto& route : routes){
        adj_list[route.first.first].push_back(std::make_pair(route.first.second, static_cast<int>(route.second)));
    }
    return adj_list;
}

std::vector<int> dijkstras(const std::string& source, const std::string& destination, std::map<std::pair<std::string, std::string>, double> routes){
    // Placeholder implementation: real algorithm to be added later.
    return {};
}
