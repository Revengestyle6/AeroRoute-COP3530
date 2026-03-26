#ifndef ROUTE_DATA_H
#define ROUTE_DATA_H

#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

std::vector<std::string> findcol(const std::filesystem::path &csvfile,
                                 const std::string &query);

std::vector<std::pair<std::string, std::string>>
findoriginnames(const std::filesystem::path &csvfile);

std::vector<std::pair<std::string, std::string>>
finddestinationnames(const std::filesystem::path &csvfile);

std::map<std::pair<std::string, std::string>, double>
findallroutes(const std::filesystem::path &csvfile);

std::map<std::pair<std::string, std::string>, double>
findoriginroutes(const std::filesystem::path &csvfile, const std::string &origin);

#endif
