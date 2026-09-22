find_package(Threads REQUIRED)

add_executable(component1_sensors 
    src/main.cpp 
    src/sensor_driver.cpp
    src/sensor_outbox.cpp
    src/sensor_sender.cpp
)
target_include_directories(component1_sensors PRIVATE include)

target_link_libraries(component1_sensors PRIVATE sfs_common nlohmann_json::nlohmann_json spdlog::spdlog)