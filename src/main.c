#include "config.h"
#include "event_logger.h"
#include "xdp_controller.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t stop_requested = 0;

static void handle_signal(int sig)
{
    (void)sig;
    stop_requested = 1;
}

int main(int argc, char **argv)
{
    const char *config_path = "config/agent.yaml";
    struct agent_config cfg;
    struct xdp_controller xdp_ctl;
    int xdp_started = 0;
    time_t last_heartbeat = 0;
    time_t last_xdp_stats = 0;

    memset(&xdp_ctl, 0, sizeof(xdp_ctl));

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (argc == 3 && strcmp(argv[1], "-c") == 0)
        config_path = argv[2];

    if (config_load(config_path, &cfg) != 0) {
        fprintf(stderr, "Failed to load config: %s\n", config_path);
        return 1;
    }

    if (logger_init(cfg.log_path) != 0) {
        fprintf(stderr, "Failed to initialize logger: %s\n", cfg.log_path);
        return 1;
    }

    logger_write("{\"event_type\":\"AGENT_STARTED\",\"severity\":\"INFO\",\"component\":\"agent\"}");

    if (cfg.xdp_enable) {
        if (xdp_controller_init(
                &xdp_ctl,
                cfg.xdp_object_path,
                cfg.xdp_interface,
                cfg.xdp_mode
            ) != 0) {
            logger_write("{\"event_type\":\"XDP_ATTACH_FAILED\",\"severity\":\"CRITICAL\",\"component\":\"xdp\"}");
            logger_close();
            return 1;
        }

        xdp_started = 1;
        logger_write("{\"event_type\":\"XDP_ATTACHED\",\"severity\":\"INFO\",\"component\":\"xdp\"}");
    }

    while (!stop_requested) {
        time_t now = time(NULL);

        if (now - last_heartbeat >= cfg.heartbeat_interval_sec) {
            logger_write("{\"event_type\":\"HEARTBEAT\",\"severity\":\"INFO\",\"component\":\"agent\"}");
            last_heartbeat = now;
        }

        if (xdp_started &&
            now - last_xdp_stats >= cfg.xdp_stats_interval_sec) {
            xdp_controller_log_stats(&xdp_ctl);
            last_xdp_stats = now;
        }

        sleep(1);
    }

    if (xdp_started) {
        xdp_controller_cleanup(&xdp_ctl);
        logger_write("{\"event_type\":\"XDP_DETACHED\",\"severity\":\"INFO\",\"component\":\"xdp\"}");
    }

    logger_write("{\"event_type\":\"AGENT_STOPPED\",\"severity\":\"INFO\",\"component\":\"agent\"}");
    logger_close();

    return 0;
}
