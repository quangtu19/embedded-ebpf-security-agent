#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static void trim_newline(char *s)
{
    size_t len = strlen(s);

    while (len > 0 && (s[len - 1] == '\n' || s[len - 1] == '\r')) {
        s[len - 1] = '\0';
        len--;
    }
}

static char *value_after_colon(char *line)
{
    char *value = strchr(line, ':');

    if (!value)
        return NULL;

    value++;
    while (*value == ' ' || *value == '\t')
        value++;

    return value;
}

static int parse_bool(const char *s)
{
    if (!s)
        return 0;

    return strcasecmp(s, "true") == 0 ||
           strcmp(s, "1") == 0 ||
           strcasecmp(s, "yes") == 0;
}

int config_load(const char *path, struct agent_config *cfg)
{
    FILE *fp;
    char line[512];

    if (!path || !cfg)
        return -1;

    memset(cfg, 0, sizeof(*cfg));

    strncpy(cfg->log_path, "/var/log/ebpf-security-agent/events.log", sizeof(cfg->log_path) - 1);
    cfg->heartbeat_interval_sec = 5;

    cfg->xdp_enable = 1;
    strncpy(cfg->xdp_interface, "enp0s3", sizeof(cfg->xdp_interface) - 1);
    strncpy(cfg->xdp_mode, "skb", sizeof(cfg->xdp_mode) - 1);
    strncpy(cfg->xdp_object_path, "/opt/ebpf-security-agent/bpf/xdp_filter.bpf.o", sizeof(cfg->xdp_object_path) - 1);
    cfg->xdp_stats_interval_sec = 5;

    fp = fopen(path, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp)) {
        char *value;

        trim_newline(line);
        value = value_after_colon(line);
        if (!value)
            continue;

        if (strstr(line, "log_path:")) {
            strncpy(cfg->log_path, value, sizeof(cfg->log_path) - 1);
        } else if (strstr(line, "heartbeat_interval_sec:")) {
            cfg->heartbeat_interval_sec = atoi(value);
            if (cfg->heartbeat_interval_sec <= 0)
                cfg->heartbeat_interval_sec = 5;
        } else if (strstr(line, "xdp_enable:")) {
            cfg->xdp_enable = parse_bool(value);
        } else if (strstr(line, "xdp_interface:")) {
            strncpy(cfg->xdp_interface, value, sizeof(cfg->xdp_interface) - 1);
        } else if (strstr(line, "xdp_mode:")) {
            strncpy(cfg->xdp_mode, value, sizeof(cfg->xdp_mode) - 1);
        } else if (strstr(line, "xdp_object_path:")) {
            strncpy(cfg->xdp_object_path, value, sizeof(cfg->xdp_object_path) - 1);
        } else if (strstr(line, "xdp_stats_interval_sec:")) {
            cfg->xdp_stats_interval_sec = atoi(value);
            if (cfg->xdp_stats_interval_sec <= 0)
                cfg->xdp_stats_interval_sec = 5;
        }
    }

    fclose(fp);
    return 0;
}
