#pragma once
#include "sensor_outbox.h"
#include <string>

class SensorSender
{
public:
    explicit SensorSender(std::string socketPath);
    ~SensorSender();
    bool connect();
    bool send(const SensorBatch &batch);
    void disconnect();

private:
    std::string socketPath_;
    int socketFd_{-1};
    bool writeAll(const std::string &payload);
};
