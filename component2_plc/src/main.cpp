#include "common/logger.h"
#include "plc_driver.h"
#include "plc_outbox.h"
#include "plc_sender.h"

#include <chrono>
#include <memory>
#include <thread>
#include <csignal>

#include <spdlog/spdlog.h>

int main() {
    std::signal(SIGPIPE, SIG_IGN);
    sfs::initializeLogging("component2");

    PLCDriver plcDriver;
    PLCOutbox outbox("component2_plc/plc_data.jsonl");
    PLCSender sender("/tmp/component2_socket");

    spdlog::info("Component 2 started");

    while (true) {
        Record record = plcDriver.readPLCRecord();

        const auto sequence = outbox.append(record);

        spdlog::debug(
            "Stored PLC record sequence={}",
            sequence
        );

        if (!sender.connect()) {
            spdlog::warn(
                "Component3 unavailable; records remain queued"
            );
        } else {
            const auto pendingRecords = outbox.pending();

            for (const Record& pendingRecord : pendingRecords) {
                if (!sender.send(pendingRecord)) {
                    break;
                }

                outbox.acknowledge(pendingRecord.sequence);
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}