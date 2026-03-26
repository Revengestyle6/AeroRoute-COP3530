#ifndef DIJKSTRAS_H
#define DIJKSTRAS_H
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
//maybe create a class for flight sim that holds all the routes
//dont load it everytime

std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adjacency_list(std::map<std::pair<std::string, std::string>, double> routes){
    std::unordered_map<std::string, std::vector<std::pair<std::string, int>>> adj_list;
    //route = key
    for (auto& route : routes){
        adj_list[route.first.first].push_back(make_pair(route.first.second, route.second));
    }
    return adj_list;
}

//dijkstras runs off of adjacency graph
std::vector<int> dijkstras(const std::string& source, const std::string& destination, std::map<std::pair<std::string, std::string>, double> routes){
    //distance array for shortest known distance from source
    //predecessor array for previous nodes in shortest path
    //visited set for nodes who's shortest distances have been finalized
    //another set for nodes that still need to be finalized/ or priority queue


}
#endif