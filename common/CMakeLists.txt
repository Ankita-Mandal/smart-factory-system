add_library(sfs_common
    src/data_generator.cpp
    src/logger.cpp
)

target_include_directories(sfs_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(sfs_common PUBLIC spdlog::spdlog)