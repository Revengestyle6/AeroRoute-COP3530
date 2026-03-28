cmake_minimum_required(VERSION 3.16)

project(AeroRoute LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(AEROROUTE_SOURCES
    main.cpp
    src/data/csv_loader.cpp
    src/data/route_data.cpp
    src/algorithms/dijkstras.cpp
    src/algorithms/a_star.cpp
)

set(AEROROUTE_HEADERS
    src/data/csv_loader.h
    src/data/route_data.h
    src/algorithms/dijkstras.h
    src/algorithms/a_star.h
)

add_executable(aeroroute
    ${AEROROUTE_SOURCES}
    ${AEROROUTE_HEADERS}
)

add_executable(aeroroute_api
    api_cli.cpp
    csv_loader.cpp
    route_data.cpp
    dijkstras.cpp
    a_star.cpp
    csv_loader.h
    route_data.h
    dijkstras.h
    a_star.h
)

target_compile_features(aeroroute PRIVATE cxx_std_17)
target_compile_features(aeroroute_api PRIVATE cxx_std_17)

set(AEROROUTE_DATA_FILES
    src/data/flight_data.csv
    src/data/accurate_airport_locations.csv
)

target_include_directories(aeroroute PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/algorithms"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/data"
)


foreach(data_file IN LISTS AEROROUTE_DATA_FILES)
    get_filename_component(data_filename "${data_file}" NAME)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${data_file}")
        add_custom_command(
            TARGET aeroroute POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/${data_file}"
                    "$<TARGET_FILE_DIR:aeroroute>/${data_filename}"
            VERBATIM
        )

        add_custom_command(
            TARGET aeroroute_api POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/${data_file}"
                    "$<TARGET_FILE_DIR:aeroroute_api>/${data_file}"
            VERBATIM
        )
    else()
        message(WARNING "Runtime data file not found: ${data_file}")
    endif()
endforeach()

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} -E chdir
            "$<TARGET_FILE_DIR:aeroroute>"
            "$<TARGET_FILE:aeroroute>"
    DEPENDS aeroroute
    USES_TERMINAL
)
