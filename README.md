# Smart Factory System

A Smart factory system simulation with:

- Component 1: sensor data processing
- Component 2: PLC data processing
- Component 3: sensor/PLC data combination
- Unix domain socket communication

## Requirements

- CMake 3.16 or newer
- C++17-compatible compiler
- Git

Dependencies are downloaded automatically by CMake:

- nlohmann/json
- spdlog

## Build

From the project root:

```bash
cmake -S . -B build
cmake --build build -j

## Clean Build
To remove generated runtime data:

rm -f sensor_data.jsonl sensor_data.jsonl.cursor
rm -f plc_data.jsonl plc_data.jsonl.cursor
rm -f combination.jsonl
rm -f component1.log component2.log component3.log


The important detail is to launch the executables from the project root because the programs use relative paths for data and log files.