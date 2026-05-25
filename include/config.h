#ifndef AGENT_CONFIG_H
#define AGENT_CONFIG_H

struct agent_config {
    char log_path[256];
    int heartbeat_interval_sec;
};

int config_load(const char *path, struct agent_config *cfg);

#endif
