cmake_minimum_required(VERSION 3.16)

project(AeroRoute LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(AEROROUTE_SOURCES
    main.cpp
    csv_loader.cpp
    route_data.cpp
    dijkstras.cpp
    a_star.cpp
)

set(AEROROUTE_HEADERS
    csv_loader.h
    route_data.h
    dijkstras.h
    a_star.h
)

add_executable(aeroroute
    ${AEROROUTE_SOURCES}
    ${AEROROUTE_HEADERS}
)

target_compile_features(aeroroute PRIVATE cxx_std_17)

set(AEROROUTE_DATA_FILES
    flight_data.csv
    accurate_airport_locations.csv
)

foreach(data_file IN LISTS AEROROUTE_DATA_FILES)
    if(EXISTS "${CMAKE_SOURCE_DIR}/${data_file}")
        add_custom_command(
            TARGET aeroroute POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_SOURCE_DIR}/${data_file}"
                    "$<TARGET_FILE_DIR:aeroroute>/${data_file}"
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
