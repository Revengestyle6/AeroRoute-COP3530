#ifndef CSV_LOADER_H
#define CSV_LOADER_H

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

std::vector<std::string> parse_csv_line(const std::string &line);

bool read_csv(const std::filesystem::path &path,
              std::vector<std::string> &header,
              std::vector<std::vector<std::string>> &rows);

std::optional<size_t> column_index(const std::vector<std::string> &header,
                                   const std::string &name);

std::string upper(const std::string &s);

std::string trim(const std::string &s);

#endif
