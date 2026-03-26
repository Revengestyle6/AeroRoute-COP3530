#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>
#include <map>
#include <iomanip>

namespace fs = std::filesystem;

// Simple CSV line parser supporting quoted fields with commas and double-quote escaping
static std::vector<std::string> parse_csv_line(const std::string &line) {
    std::vector<std::string> result;
    std::string cur;
    bool in_quotes = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (in_quotes) {
            if (c == '"') {
                if (i + 1 < line.size() && line[i + 1] == '"') {
                    // Escaped quote
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

static std::string upper(const std::string &s) {
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return std::toupper(c); });
    return out;
}

// Read CSV and return header->index map and vector of rows (each row is vector<string>)
static bool read_csv(const fs::path &path, std::vector<std::string> &header, std::vector<std::vector<std::string>> &rows) {
    std::ifstream in(path);
    if (!in) return false;
    std::string line;
    if (!std::getline(in, line)) return false;
    header = parse_csv_line(line);
    while (std::getline(in, line)) {
        rows.push_back(parse_csv_line(line));
    }
    return true;
}

static std::optional<size_t> column_index(const std::vector<std::string> &header, const std::string &name) {
    for (size_t i = 0; i < header.size(); ++i) {
        if (header[i] == name) return i;
    }
    return std::nullopt;
}

// Equivalent of findcol(csvfile, query)
static std::vector<std::string> findcol(const fs::path &csvfile, const std::string &query) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    auto idx = column_index(header, query);
    if (!idx) throw std::runtime_error("Column not found: " + query);
    std::set<std::string> output;
    for (const auto &r : rows) {
        if (*idx < r.size()) {
            const std::string &value = r[*idx];
            if (value.empty()) continue;
            output.insert(upper(value));
        }
    }
    return std::vector<std::string>(output.begin(), output.end());
}

// findoriginnames -> returns vector of pairs (airport_upper, geocode_string)
static std::vector<std::pair<std::string, std::string>> findoriginnames(const fs::path &csvfile) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    auto idx_city = column_index(header, "airport_1");
    auto idx_geo = column_index(header, "Geocoded_City1");
    if (!idx_city || !idx_geo) throw std::runtime_error("Required columns not found (city1 / Geocoded_City1)");
    std::set<std::pair<std::string, std::string>> output;
    for (const auto &r : rows) {
        std::string airport = (*idx_city < r.size()) ? r[*idx_city] : std::string();
        std::string geocode = (*idx_geo < r.size()) ? r[*idx_geo] : std::string();
        if (!airport.empty() && !geocode.empty()) {
            // strip whitespace from geocode
            auto s = geocode;
            // trim
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            output.insert({upper(airport), s});
        }
    }
    return std::vector<std::pair<std::string, std::string>>(output.begin(), output.end());
}

static std::vector<std::pair<std::string, std::string>> finddestinationnames(const fs::path &csvfile) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    auto idx_city = column_index(header, "airport_2");
    auto idx_geo = column_index(header, "Geocoded_City2");
    if (!idx_city || !idx_geo) throw std::runtime_error("Required columns not found (city2 / Geocoded_City2)");
    std::set<std::pair<std::string, std::string>> output;
    for (const auto &r : rows) {
        std::string airport = (*idx_city < r.size()) ? r[*idx_city] : std::string();
        std::string geocode = (*idx_geo < r.size()) ? r[*idx_geo] : std::string();
        if (!airport.empty() && !geocode.empty()) {
            auto s = geocode;
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            output.insert({upper(airport), s});
        }
    }
    return std::vector<std::pair<std::string, std::string>>(output.begin(), output.end());
}

// Return map of (origin,destination) -> distance (nsmiles), store both directions like main.py
static std::map<std::pair<std::string, std::string>, double> findallroutes(const fs::path &csvfile) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    auto idx_o = column_index(header, "airport_1");
    auto idx_d = column_index(header, "airport_2");
    auto idx_dist = column_index(header, "nsmiles");
    if (!idx_o || !idx_d || !idx_dist) throw std::runtime_error("Required columns not found (airport_1 / airport_2 / nsmiles)");
    std::map<std::pair<std::string, std::string>, double> output;
    for (const auto &r : rows) {
        std::string origin = (*idx_o < r.size()) ? r[*idx_o] : std::string();
        std::string dest = (*idx_d < r.size()) ? r[*idx_d] : std::string();
        std::string dist_s = (*idx_dist < r.size()) ? r[*idx_dist] : std::string();
        if (!origin.empty() && !dest.empty()) {
            try {
                double dist = dist_s.empty() ? 0.0 : std::stod(dist_s);
                auto o_up = upper(origin);
                auto d_up = upper(dest);
                output[{o_up, d_up}] = dist;
                output[{d_up, o_up}] = dist;
            } catch (...) {
                // ignore parse errors and continue
                continue;
            }
        }
    }
    return output;
}
static std::map<std::pair<std::string, std::string>, double> findoriginroutes(const fs::path &csvfile, const std::string &origin) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    auto idx_o = column_index(header, "airport_1");
    auto idx_d = column_index(header, "airport_2");
    auto idx_dist = column_index(header, "nsmiles");
    if (!idx_o || !idx_d || !idx_dist) throw std::runtime_error("Required columns not found (airport_1 / airport_2 / nsmiles)");
    std::map<std::pair<std::string, std::string>, double> output;
    std::string origin_up = upper(origin);
    for (const auto &r : rows) {
        std::string row_origin = (*idx_o < r.size()) ? r[*idx_o] : std::string();
        std::string dest = (*idx_d < r.size()) ? r[*idx_d] : std::string();
        std::string dist_s = (*idx_dist < r.size()) ? r[*idx_dist] : std::string();
        if (!row_origin.empty() && !dest.empty()) {
            if (upper(row_origin) == origin_up) {
                try {
                    double dist = dist_s.empty() ? 0.0 : std::stod(dist_s);
                    output[{upper(row_origin), upper(dest)}] = dist;
                } catch (...) { continue; }
            }
        }
    }
    return output;
}

int main(int argc, char **argv) {
    try {
        fs::path base = fs::current_path();
        fs::path csv_path = base / "flight_data.csv";
        if (!fs::exists(csv_path)) {
            std::cerr << "CSV file not found at: " << csv_path << "\n";
            return 1;
        }

        auto origins = findoriginnames(csv_path);
        std::cout << "Found " << origins.size() << " origin airports (showing up to 20):\n";
        size_t shown = 0;
        for (const auto &p : origins) {
            std::cout << p.first << " -> '" << p.second << "'\n";
            if (++shown >= 20) break;
        }

        auto routes = findallroutes(csv_path);
        std::cout << "Total unique route pairs stored (both directions): " << routes.size() << "\n";
        // show a few sample routes with distances
        size_t rshown = 0;
        for (const auto &kv : routes) {
            const auto &pair = kv.first;
            double dist = kv.second;
            std::cout << pair.first << " -> " << pair.second << " : " << std::fixed << std::setprecision(2) << dist << "\n";
            if (++rshown >= 10) break;
        }
        return 0;
    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return 2;
    }
}
