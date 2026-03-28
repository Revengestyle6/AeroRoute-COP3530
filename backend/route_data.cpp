#include "route_data.h"

#include <iostream>

#include "csv_loader.h"

#include <set>
#include <stdexcept>

std::vector<std::string> findcol(const std::filesystem::path &csvfile,
                                 const std::string &query) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) {
        throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    }

    auto idx = column_index(header, query);
    if (!idx) {
        throw std::runtime_error("Column not found: " + query);
    }

    std::set<std::string> output;
    for (const auto &row : rows) {
        if (*idx < row.size()) {
            const std::string &value = row[*idx];
            if (!value.empty()) {
                output.insert(upper(value));
            }
        }
    }

    return std::vector<std::string>(output.begin(), output.end());
}

std::vector<std::pair<std::string, std::string>>
findoriginnames(const std::filesystem::path &csvfile) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) {
        throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    }

    auto idx_city = column_index(header, "airport_1");
    auto idx_geo = column_index(header, "Geocoded_City1");
    if (!idx_city || !idx_geo) {
        throw std::runtime_error("Required columns not found (city1 / Geocoded_City1)");
    }

    std::set<std::pair<std::string, std::string>> output;
    for (const auto &row : rows) {
        std::string airport = (*idx_city < row.size()) ? row[*idx_city] : std::string();
        std::string geocode = (*idx_geo < row.size()) ? row[*idx_geo] : std::string();
        if (!airport.empty() && !geocode.empty()) {
            output.insert({upper(airport), trim(geocode)});
        }
    }

    return std::vector<std::pair<std::string, std::string>>(output.begin(), output.end());
}

std::vector<std::pair<std::string, std::string>>
finddestinationnames(const std::filesystem::path &csvfile) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) {
        throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    }

    auto idx_city = column_index(header, "airport_2");
    auto idx_geo = column_index(header, "Geocoded_City2");
    if (!idx_city || !idx_geo) {
        throw std::runtime_error("Required columns not found (city2 / Geocoded_City2)");
    }

    std::set<std::pair<std::string, std::string>> output;

    for (const auto &row : rows) {
        std::string airport = (*idx_city < row.size()) ? row[*idx_city] : std::string();
        std::string geocode = (*idx_geo < row.size()) ? row[*idx_geo] : std::string();
        if (!airport.empty() && !geocode.empty()) {
            output.insert({upper(airport), trim(geocode)});
        }
    }

    return std::vector<std::pair<std::string, std::string>>(output.begin(), output.end());
}

std::map<std::pair<std::string, std::string>, int>
findallroutes(const std::filesystem::path &csvfile) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) {
        throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    }

    auto idx_origin = column_index(header, "airport_1");
    auto idx_destination = column_index(header, "airport_2");
    auto idx_distance = column_index(header, "nsmiles");
    if (!idx_origin || !idx_destination || !idx_distance) {
        throw std::runtime_error("Required columns not found (airport_1 / airport_2 / nsmiles)");
    }

    std::map<std::pair<std::string, std::string>, int> output;
    for (const auto &row : rows) {
        std::string origin = (*idx_origin < row.size()) ? row[*idx_origin] : std::string();
        std::string destination = (*idx_destination < row.size()) ? row[*idx_destination] : std::string();
        std::string distance_string = (*idx_distance < row.size()) ? row[*idx_distance] : std::string();
        if (!origin.empty() && !destination.empty()) {
            try {
                int distance = distance_string.empty() ? 0.0 : std::stod(distance_string);
                std::string origin_upper = upper(origin);
                std::string destination_upper = upper(destination);
                output[{origin_upper, destination_upper}] = distance;
                output[{destination_upper, origin_upper}] = distance;
                std::cout << "Finding all routes..." << std::endl;
            } catch (...) {
                continue;
            }
        }
    }

    return output;
}

std::map<std::pair<std::string, std::string>, int>
findoriginroutes(const std::filesystem::path &csvfile, const std::string &origin) {
    std::vector<std::string> header;
    std::vector<std::vector<std::string>> rows;
    if (!read_csv(csvfile, header, rows)) {
        throw std::runtime_error("Unable to read CSV: " + csvfile.string());
    }

    auto idx_origin = column_index(header, "airport_1");
    auto idx_destination = column_index(header, "airport_2");
    auto idx_distance = column_index(header, "nsmiles");
    if (!idx_origin || !idx_destination || !idx_distance) {
        throw std::runtime_error("Required columns not found (airport_1 / airport_2 / nsmiles)");
    }

    std::map<std::pair<std::string, std::string>, int> output;
    std::string origin_upper = upper(origin);

    for (const auto &row : rows) {
        std::string row_origin = (*idx_origin < row.size()) ? row[*idx_origin] : std::string();
        std::string destination = (*idx_destination < row.size()) ? row[*idx_destination] : std::string();
        std::string distance_string = (*idx_distance < row.size()) ? row[*idx_distance] : std::string();
        if (!row_origin.empty() && !destination.empty() && upper(row_origin) == origin_upper) {
            try {
                output[{upper(row_origin), upper(destination)}] =
                    distance_string.empty() ? 0.0 : std::stod(distance_string);
            } catch (...) {
                continue;
            }
        }
    }

    return output;
}
