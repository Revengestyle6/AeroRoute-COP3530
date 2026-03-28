cmake_minimum_required(VERSION 3.16)

project(AeroRoute LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(aeroroute
    main.cpp
    csv_loader.cpp
    route_data.cpp
    dijkstras.cpp
        a_star.cpp
        a_star.h
)

target_compile_features(aeroroute PRIVATE cxx_std_17)

add_custom_command(
    TARGET aeroroute POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/flight_data.csv
            $<TARGET_FILE_DIR:aeroroute>/flight_data.csv
)
