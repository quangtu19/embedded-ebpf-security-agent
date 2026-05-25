#ifndef AGENT_CONFIG_H
#define AGENT_CONFIG_H

struct agent_config {
    char log_path[256];
    int heartbeat_interval_sec;

    int xdp_enable;
    char xdp_interface[64];
    char xdp_mode[32];
    char xdp_object_path[256];
    int xdp_stats_interval_sec;
};

int config_load(const char *path, struct agent_config *cfg);

#endif
