#include <iostream>
#include <cstring>
#include <cstdio>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

namespace {
    constexpr const char* SensorSocketPath = "/tmp/component1_socket";
    constexpr const char* PlcSocketPath = "/tmp/component2_socket";
    constexpr std::size_t BufferSize = 1024;

    int createListeningSocket(const char* socketPath) {

        //remove any stale socket file before creating a new one
        ::unlink(socketPath);

        int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (sockfd == -1) {
            perror("socket");
            return -1;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path) - 1);

        unlink(socketPath); // Remove any existing socket file

        if (bind(sockfd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
            perror("bind");
            close(sockfd);
            return -1;
        }

        if (listen(sockfd, 5) == -1) {
            perror("listen");
            close(sockfd);
            return -1;
        }

        return sockfd;
    }

    int acceptConnection(int listeningSockfd, const char* label) {
        std::cout << "[component3] waiting for connection from " << label << "...\n";
        int clientSockfd = accept(listeningSockfd, nullptr, nullptr);
        if (clientSockfd == -1) {
            perror("accept");
            return -1;
        }
        std::cout << "[component3] connected to " << label << "\n";
        return clientSockfd;
    }
}

int main() {
    int sensorListeningSockfd = createListeningSocket(SensorSocketPath);
    if (sensorListeningSockfd == -1) {
        std::cerr << "[component3] failed to create listening socket for component1\n";
        return 1;
    }

    int plcListeningSockfd = createListeningSocket(PlcSocketPath);
    if (plcListeningSockfd == -1) {
        std::cerr << "[component3] failed to create listening socket for component2\n";
        return 1;
    }

    int sensorClientSockfd = acceptConnection(sensorListeningSockfd, "component1");
    if (sensorClientSockfd == -1) {
        return 1;
    }

    int plcClientSockfd = acceptConnection(plcListeningSockfd, "component2");
    if (plcClientSockfd == -1) {
        return 1;
    }

    pollfd fds[2];
    fds[0].fd = sensorClientSockfd;
    fds[0].events = POLLIN;
    fds[1].fd = plcClientSockfd;
    fds[1].events = POLLIN;
    char buffer[BufferSize];

    std::cout << "[component3] ready to receive data from component1 and component2\n";
    while(true){
        int ready = poll(fds, 2, -1); // Wait indefinitely for events
        if (ready == -1) {
            perror("poll");
            break;
        }

        for (auto& pfd : fds) {
            if (pfd.fd < 0 || !(pfd.revents & POLLIN)) {
                continue;
            }
 
            ssize_t n = ::read(pfd.fd, buffer, sizeof(buffer) - 1);
            const char* label = (pfd.fd == sensorClientSockfd) ? "sensor" : "plc";
 
            if (n > 0) {
                buffer[n] = '\0';
                std::cout << "[component3] received from " << label << ": " << buffer << "\n";
            } else if (n == 0) {
                std::cout << "[component3] " << label << " disconnected\n";
                pfd.fd = -1;  // stop polling this one
            } else {
                std::perror("read");
            }
        }
    }
    ::close(sensorClientSockfd);
    ::close(plcClientSockfd);
    ::close(sensorListeningSockfd);
    ::close(plcListeningSockfd);
    return 0;
}