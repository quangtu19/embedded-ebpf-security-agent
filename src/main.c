#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "event_logger.h"
#include "config.h"
#include "ebpf_loader.h"

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

int main(int argc, char **argv)
{
    const char *config_path = "config/agent.yaml";
    const char *bpf_object_path = "bpf/exec_trace.bpf.o";
    struct agent_config cfg;
    int heartbeat_elapsed_ms = 0;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (argc == 3 && strcmp(argv[1], "-c") == 0) {
        config_path = argv[2];
    }

    if (config_load(config_path, &cfg) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", config_path);
        return 1;
    }

    if (logger_init(cfg.log_path) != 0) {
        fprintf(stderr, "Failed to initialize logger: %s\n", cfg.log_path);
        return 1;
    }

    logger_write("{\"event_type\":\"AGENT_STARTED\",\"severity\":\"INFO\"}");

    if (ebpf_loader_start(bpf_object_path) != 0) {
        logger_write("{\"event_type\":\"BPF_LOAD_FAILED\",\"severity\":\"CRITICAL\"}");
        logger_close();
        return 1;
    }

    logger_write("{\"event_type\":\"BPF_LOADED\",\"severity\":\"INFO\",\"program\":\"exec_trace\"}");

    while (!stop_requested) {
        ebpf_loader_poll(500);

        heartbeat_elapsed_ms += 500;
        if (heartbeat_elapsed_ms >= cfg.heartbeat_interval_sec * 1000) {
            logger_write("{\"event_type\":\"HEARTBEAT\",\"severity\":\"INFO\"}");
            heartbeat_elapsed_ms = 0;
        }
    }

    logger_write("{\"event_type\":\"AGENT_STOPPED\",\"severity\":\"INFO\"}");

    ebpf_loader_stop();
    logger_close();

    return 0;
}
