find_package(Threads REQUIRED)

add_executable(component2_plc
    src/main.cpp
    src/plc_driver.cpp
    src/plc_outbox.cpp
    src/plc_sender.cpp
)

target_include_directories(component2_plc PRIVATE include)

target_link_libraries(component2_plc PRIVATE sfs_common nlohmann_json::nlohmann_json spdlog::spdlog)
