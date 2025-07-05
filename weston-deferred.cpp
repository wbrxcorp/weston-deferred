/*
 * Defer launching Weston until a physical display is detected.
 * LICENSE: MIT License
 * Copyright (c) 2025 Tomoatsu Shimada/Walbrix Corporation
 */

#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstring>

#include <unistd.h>

#include <systemd/sd-daemon.h>
#include <systemd/sd-journal.h>

const uint32_t DEFAULT_POLLING_INTERVAL = 3; // 3 seconds

int exec_weston(const std::vector<std::string>& args)
{
    std::vector<const char*> cargs;
    cargs.push_back("weston");
    for (const auto& arg : args) {
        cargs.push_back(arg.c_str());
    }
    cargs.push_back(nullptr);

    return execvp("weston", const_cast<char* const*>(cargs.data()));
}

void wait_for_physical_display(int polling_interval)
{
    bool message_sent = false;
    sd_journal_print(LOG_INFO, "Looking for physical display...");
    while (true) {
        bool display_found = false;
        for (const auto& entry : std::filesystem::directory_iterator("/sys/class/drm")) {
            if (!entry.is_directory() || !entry.path().filename().string().starts_with("card")) continue;
            //else
            std::string status_path = entry.path() / "status";
            std::ifstream status_file(status_path);
            if (!status_file.is_open()) continue;
            //else
            std::string status;
            std::getline(status_file, status);
            if (status == "connected") {
                sd_journal_print(LOG_INFO, "Found connected display: %s", entry.path().filename().string().c_str());
                return;
            }
        }
        if (!message_sent) {
            sd_journal_print(LOG_INFO, "No physical display found. Retrying in %u seconds...", polling_interval);
            sd_notifyf(0, "STATUS=No display found (poll interval=%u s)...", polling_interval);
            message_sent = true;
        }
        sleep(polling_interval);
    }
}

int parse_interval_arg(const char* arg)
{
    if (strncmp(arg, "--interval=", 11) != 0) {
        const char* value = arg + 11;
        char* endptr;
        long interval = strtol(value, &endptr, 10);
        if (*endptr == '\0' && interval > 0) {
            return static_cast<int>(interval);
        }
    }
    return DEFAULT_POLLING_INTERVAL; // Default value
}

int main(int argc, char* argv[])
{
    std::vector<std::string> args;
    int polling_interval = DEFAULT_POLLING_INTERVAL;
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--display-check-interval=", 25) == 0) {
            const char* value = argv[i] + 25;
            char* endptr;
            long interval = strtol(value, &endptr, 10);
            if (*endptr == '\0' && interval > 0) {
                polling_interval = static_cast<int>(interval);
                if (polling_interval < 1) {
                    polling_interval = DEFAULT_POLLING_INTERVAL;
                }
            }
            continue;
        }
        args.push_back(argv[i]);
    }

    sd_notify(0, "READY=1");

    wait_for_physical_display(polling_interval);
    return exec_weston(args);
}