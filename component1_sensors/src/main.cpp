// main function to read from both types of sensors once per second. Each reading produces a record consisting of a timestamp and an vector of double data values. The records are stored in a vector and sent to component 3 for further processing.
//get data size and range from yaml config

#include "sensor_driver.h"
#include "common/data_generator.h"
// #include <yaml-cpp/yaml.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <chrono>
#include <thread>
#include <sys/socket.h>
#include <sys/un.h>

namespace {
    constexpr const char* SOCKET_PATH = "/tmp/component1_socket";

    int connectWithRetry(const char* socketPath) {
        int sockfd;
        struct sockaddr_un addr;

        while (true) {
            sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
            if (sockfd == -1) {
                perror("socket");
                return -1;
            }

            memset(&addr, 0, sizeof(addr));
            addr.sun_family = AF_UNIX;
            strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

            // if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
            //     perror("connect");
            //     close(sockfd);
            //     std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
            //     continue;
            // }
            if (::connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == 0) {
                std::cout << "[component1] connected to component3 at " << socketPath << "\n";
                return sockfd;
            }
 
            std::cout << "[component1] component3 not available yet, retrying in 1s...\n";
            ::close(sockfd);
            
            std::this_thread::sleep_for(std::chrono::seconds(1));

        }
    }
}

int main(){
    int sockfd = connectWithRetry(SOCKET_PATH);
    if (sockfd == -1) { 
        std::cerr << "[component1] failed to connect to component3\n";
        return 1;
    }
    int tick = 0;
    while (true) {
        // Simulate reading from sensors and generating records
        std::string msg = "sensor_tick_" + std::to_string(tick++);
        ssize_t sent = ::write(sockfd, msg.c_str(), msg.size());
        if (sent < 0) {
            std::perror("write");
            break;
        }
        std::cout << "[component1] sent: " << msg << "\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
 
    ::close(sockfd);
    return 0;
}