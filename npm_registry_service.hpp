#pragma once
// std

// lib
#include "npm_registry.hpp"

// deps
#include <logging/logger>
#include <service>

namespace npm {

struct registry_service_configuration {
    std::string ip_address;
    int port;
};

class registry_service : public svc::service_base {
  public:
    registry_service() : service_base("NpmRegistryService") {}
    registry_service(registry_service_configuration* config, logging::logger* logger)
        : service_base("NpmRegistryService"), logger_(logger) {
        use_ip_and_port(config->ip_address, config->port);
    }
    registry_service(const string& ip, int port) : service_base("NpmRegistryService") {
        use_ip_and_port(!ip.empty() ? ip : listening_ip,
                        port > 0 && port < 65535 ? port : listening_port);
    }

    void use_ip_and_port(const string& ip, int port) {
        use_ip(ip);
        use_port(port);
    }

    void use_ip(const string& ip) { listening_ip = ip; }

    void use_port(int port) {
        if (port < 1 || port > 65535)
            throw std::invalid_argument("Port number must be between 1-65535");
        listening_port = port;
    }

  protected:
    std::string listening_ip = "127.0.0.1";
    int listening_port       = 8080;
    std::unique_ptr<registry> npm_registry;
    std::thread registry_thread;
    logging::logger* logger_ = nullptr;

    void main_loop() override {
        npm_registry = std::make_unique<registry>(listening_ip, listening_port);

        log_event(EVENTLOG_INFORMATION_TYPE, "Starting HTTP server...");

        try {
            npm_registry->start();
        } catch (const std::exception& ex) {
            log_event(EVENTLOG_ERROR_TYPE,
                      string("Server error: ") + string(ex.what(), ex.what() + strlen(ex.what())));
        }

        if (service_mode) {
            while (running) {
                Sleep(1000);
            }

            if (npm_registry) {
                log_event(EVENTLOG_INFORMATION_TYPE, "Stopping HTTP server...");
                npm_registry->stop();
            }

            log_event(EVENTLOG_INFORMATION_TYPE, "Service main loop exited.");
        }
    }
};

} // namespace npm