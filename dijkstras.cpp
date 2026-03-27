#include "dijkstras.h"

#include <iostream>
#include <utility>

std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(std::map<std::pair<std::string, std::string>, double> routes){
    std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adj_list;
    for (auto& route : routes){
        adj_list[route.first.first].emplace_back(route.first.second, static_cast<int>(route.second));
    }
    // TEST TO SHOW ADJACENCY LIST WORKING PROPERLY.
    // for (auto& item : adj_list) {
    //     std::cout << item.first << std::endl;
    //     for (auto& dest : item.second) {
    //         std::cout << dest.first << dest.second << std::endl;
    //     }
    // }
    return adj_list;
}

std::vector<int> dijkstras(const std::string& source, const std::string& destination, std::map<std::pair<std::string, std::string>, double> routes){
    // Placeholder implementation: real algorithm to be added later.
    return {};
}
