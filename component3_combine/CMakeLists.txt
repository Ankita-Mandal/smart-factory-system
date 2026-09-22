add_executable(component3_combine src/main.cpp)

target_include_directories(component3_combine PRIVATE include)

target_link_libraries(component3_combine PRIVATE sfs_common nlohmann_json::nlohmann_json spdlog::spdlog)
