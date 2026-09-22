
#pragma once
#include "common/record.h"
#include<string>


class PLCSender {
public:
    explicit PLCSender(std::string socketPath);
    ~PLCSender();
    bool connect();
    bool send(const Record& record);
    void disconnect();

private:
    std::string socketPath_;
    int socketFd_{-1};
    bool writeAll(const std::string& payload);
};