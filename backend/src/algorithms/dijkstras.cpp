#include "dijkstras.h"
#include <algorithm>
#include <climits>
#include <utility>
#include <queue>

std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(const std::map<std::pair<std::string, std::string>, int>& routes){
    std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adj_list;
    for (auto& route : routes){
        adj_list[route.first.first].emplace_back(route.first.second, static_cast<int>(route.second));
    }

    return adj_list;
}

std::pair<int, std::vector<std::string>> dijkstras(const std::string& source, const std::string& destination, std::unordered_map<std::string, std::vector<std::pair<std::string, int>>>& routes) {
    std::priority_queue<std::pair<int, std::string>, std::vector<std::pair<int, std::string>>, std::greater<std::pair<int,std::string>>> unfinished;
    std::unordered_map<std::string, int> distance;
    std::unordered_map<std::string, std::string> predecessor;

    //initialize finished list
    distance[source] = 0;
    predecessor[source] = "N/A";
    unfinished.push(make_pair(distance[source], source));
    //initialize unfinished list
    for (const auto& origin : routes) {
        if (origin.first != source) {
            distance[origin.first] = INT_MAX;
            predecessor[origin.first] = "N/A";
        }
    }

    std::string smallest;
    int min;
    while (!unfinished.empty()) {
        min = INT_MAX;
        // for (const auto& x : unfinished) {
        //     if (distance[x] < min) {
        //         min = distance[x];
        //         smallest = x;
        //     }
        // }
        smallest = unfinished.top().second;
        min = unfinished.top().first;
        unfinished.pop();
        if (min > distance[smallest]) 
            continue;

        for (const auto& route : routes[smallest]) {
            if (distance[smallest] + route.second < distance[route.first]) {
                distance[route.first] = distance[smallest] + route.second;
                unfinished.push(make_pair(distance[route.first], route.first));
                predecessor[route.first] = smallest;
            }
        }
    }
    int outputDistance = distance[destination];
    std::vector<std::string> path = {};
    std::string curr = destination;
    path.push_back(destination);
    while (predecessor[curr] != "N/A") {
        path.insert(path.begin(), predecessor[curr]);
        curr = predecessor[curr];
    }

    return std::make_pair(outputDistance, path);
}
