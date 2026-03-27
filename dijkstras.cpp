#include "dijkstras.h"
#include <algorithm>
#include <iostream>
#include <utility>

std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(const std::map<std::pair<std::string, std::string>, int>& routes){
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

std::pair<int, std::vector<std::string>> dijkstras(const std::string& source, const std::string& destination, std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes) {
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
        std::cout << "initializing dijkstras" << std::endl;
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
        std::cout << "running dijkstras" << std::endl;
        std::string smallest;
        int min = INT_MAX;
        for (const auto& x : unfinished) {
            if (distance[x] < min) {
                min = distance[x];
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
