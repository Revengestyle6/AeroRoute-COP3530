#include "csv_loader.h"

#include <algorithm>
#include <cctype>
#include <fstream>

std::vector<std::string> parse_csv_line(const std::string &line) {
    std::vector<std::string> result;
    std::string cur;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    cur.push_back('"');
                    ++i;
                } else {
                    in_quotes = false;
                }
            } else {
                cur.push_back(c);
            }
        } else {
            if (c == '"') {
                in_quotes = true;
            } else if (c == ',') {
                result.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(c);
            }
        }
    }

    result.push_back(cur);
    return result;
}

bool read_csv(const std::filesystem::path &path,
              std::vector<std::string> &header,
              std::vector<std::vector<std::string>> &rows) {
    std::ifstream in(path);
    if (!in) {
        return false;
    }

    std::string line;
    if (!std::getline(in, line)) {
        return false;
    }

    header = parse_csv_line(line);
    while (std::getline(in, line)) {
        rows.push_back(parse_csv_line(line));
    }

    return true;
}

std::optional<size_t> column_index(const std::vector<std::string> &header,
                                   const std::string &name) {
    for (size_t i = 0; i < header.size(); ++i) {
        if (header[i] == name) {
            return i;
        }
    }
    return std::nullopt;
}

std::string upper(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return out;
}

std::string trim(const std::string &s) {
    auto start = std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    });
    auto end = std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base();

    if (start >= end) {
        return "";
    }

    return std::string(start, end);
}
