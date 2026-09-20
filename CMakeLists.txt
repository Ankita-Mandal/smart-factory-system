cmake_minimum_required(VERSION 3.16)
project(SmartFactorySystem CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(component1_sensors)
add_subdirectory(component2_plc)
add_subdirectory(component3_combine)
add_subdirectory(common) 
