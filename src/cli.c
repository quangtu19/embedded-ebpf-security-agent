#include "common_bpf_keys.h"

#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define PIN_DIR "/sys/fs/bpf/ebpf-security-agent"
#define PIN_PROTOCOL_MAP PIN_DIR "/protocol_count_map"
#define PIN_PACKET_MAP PIN_DIR "/packet_count_map"
#define PIN_BLACKLIST_MAP PIN_DIR "/blacklist_map"

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage:\n"
        "  sudo %s status\n"
        "  sudo %s stats\n"
        "  sudo %s report\n"
        "  sudo %s blacklist add <IPv4>\n"
        "  sudo %s blacklist remove <IPv4>\n"
        "  sudo %s blacklist list\n",
        prog, prog, prog, prog, prog, prog);
}

static int open_map(const char *path)
{
    int fd = bpf_obj_get(path);

    if (fd < 0)
        fprintf(stderr, "failed to open pinned map %s: %s\n", path, strerror(errno));

    return fd;
}

static int parse_ipv4(const char *ip, uint32_t *key)
{
    struct in_addr addr;

    if (inet_pton(AF_INET, ip, &addr) != 1) {
        fprintf(stderr, "Invalid IPv4 address: %s\n", ip);
        return -1;
    }

    *key = addr.s_addr;
    return 0;
}

static uint64_t read_counter(int fd, uint32_t key)
{
    uint64_t value = 0;

    bpf_map_lookup_elem(fd, &key, &value);
    return value;
}

static int find_agent_pid(void)
{
    DIR *dir;
    struct dirent *ent;

    dir = opendir("/proc");
    if (!dir)
        return -1;

    while ((ent = readdir(dir)) != NULL) {
        char path[256];
        char cmdline[512];
        FILE *fp;
        size_t n;

        if (ent->d_name[0] < '0' || ent->d_name[0] > '9')
            continue;

        snprintf(path, sizeof(path), "/proc/%s/cmdline", ent->d_name);
        fp = fopen(path, "r");
        if (!fp)
            continue;

        n = fread(cmdline, 1, sizeof(cmdline) - 1, fp);
        fclose(fp);

        if (n == 0)
            continue;

        cmdline[n] = '\0';

        for (size_t i = 0; i < n; i++) {
            if (cmdline[i] == '\0')
                cmdline[i] = ' ';
        }

        if (strstr(cmdline, "ebpf-security-agent") != NULL &&
            strstr(cmdline, "ebpf-agentctl") == NULL) {
            int pid = atoi(ent->d_name);
            closedir(dir);
            return pid;
        }
    }

    closedir(dir);
    return -1;
}

static int cmd_status(void)
{
    int pid;
    int protocol_fd;
    int packet_fd;
    int blacklist_fd;

    pid = find_agent_pid();

    protocol_fd = open_map(PIN_PROTOCOL_MAP);
    packet_fd = open_map(PIN_PACKET_MAP);
    blacklist_fd = open_map(PIN_BLACKLIST_MAP);

    printf("state=%s\n", pid > 0 ? "RUNNING" : "UNKNOWN_OR_STOPPED");
    printf("pid=%d\n", pid > 0 ? pid : -1);
    printf("pin_dir=%s\n", PIN_DIR);
    printf("protocol_count_map=%s\n", protocol_fd >= 0 ? "ok" : "missing");
    printf("packet_count_map=%s\n", packet_fd >= 0 ? "ok" : "missing");
    printf("blacklist_map=%s\n", blacklist_fd >= 0 ? "ok" : "missing");
    printf("maps=%s\n",
           protocol_fd >= 0 && packet_fd >= 0 && blacklist_fd >= 0 ? "ok" : "missing");
    printf("xdp_iface=enp0s8\n");

    if (protocol_fd >= 0)
        close(protocol_fd);
    if (packet_fd >= 0)
        close(packet_fd);
    if (blacklist_fd >= 0)
        close(blacklist_fd);

    return 0;
}

static int cmd_stats(void)
{
    int protocol_fd;
    int packet_fd;

    protocol_fd = open_map(PIN_PROTOCOL_MAP);
    if (protocol_fd < 0)
        return 1;

    packet_fd = open_map(PIN_PACKET_MAP);
    if (packet_fd < 0) {
        close(protocol_fd);
        return 1;
    }

    printf("tcp=%llu udp=%llu icmp=%llu other=%llu total=%llu bytes=%llu dropped=%llu\n",
           (unsigned long long)read_counter(protocol_fd, PROTO_TCP),
           (unsigned long long)read_counter(protocol_fd, PROTO_UDP),
           (unsigned long long)read_counter(protocol_fd, PROTO_ICMP),
           (unsigned long long)read_counter(protocol_fd, PROTO_OTHER),
           (unsigned long long)read_counter(packet_fd, STAT_TOTAL_PACKETS),
           (unsigned long long)read_counter(packet_fd, STAT_TOTAL_BYTES),
           (unsigned long long)read_counter(packet_fd, STAT_DROPPED));

    close(protocol_fd);
    close(packet_fd);
    return 0;
}

static int cmd_blacklist_add(const char *ip)
{
    int fd;
    uint32_t key;
    uint8_t value = 1;

    if (parse_ipv4(ip, &key) != 0)
        return 1;

    fd = open_map(PIN_BLACKLIST_MAP);
    if (fd < 0)
        return 1;

    if (bpf_map_update_elem(fd, &key, &value, BPF_ANY) != 0) {
        fprintf(stderr, "BLACKLIST_ADD ip=%s result=failed error=%s\n", ip, strerror(errno));
        close(fd);
        return 1;
    }

    printf("BLACKLIST_ADD ip=%s result=ok\n", ip);
    close(fd);
    return 0;
}

static int cmd_blacklist_remove(const char *ip)
{
    int fd;
    uint32_t key;

    if (parse_ipv4(ip, &key) != 0)
        return 1;

    fd = open_map(PIN_BLACKLIST_MAP);
    if (fd < 0)
        return 1;

    if (bpf_map_delete_elem(fd, &key) != 0 && errno != ENOENT) {
        fprintf(stderr, "BLACKLIST_REMOVE ip=%s result=failed error=%s\n", ip, strerror(errno));
        close(fd);
        return 1;
    }

    printf("BLACKLIST_REMOVE ip=%s result=ok\n", ip);
    close(fd);
    return 0;
}

static int cmd_blacklist_list(void)
{
    int fd;
    uint32_t key;
    uint32_t next_key;
    int has_key = 0;
    int count = 0;

    fd = open_map(PIN_BLACKLIST_MAP);
    if (fd < 0)
        return 1;

    while (bpf_map_get_next_key(fd, has_key ? &key : NULL, &next_key) == 0) {
        struct in_addr addr;
        char ip[INET_ADDRSTRLEN];

        addr.s_addr = next_key;
        inet_ntop(AF_INET, &addr, ip, sizeof(ip));
        printf("%s\n", ip);

        key = next_key;
        has_key = 1;
        count++;
    }

    if (count == 0)
        printf("blacklist is empty\n");

    close(fd);
    return 0;
}

static int cmd_report(void)
{
    int pid = find_agent_pid();

    printf("{\n");
    printf("  \"agent_status\": \"%s\",\n", pid > 0 ? "RUNNING" : "UNKNOWN_OR_STOPPED");
    printf("  \"pid\": %d,\n", pid > 0 ? pid : -1);
    printf("  \"pin_dir\": \"%s\",\n", PIN_DIR);
    printf("  \"stats\": \"");
    fflush(stdout);
    cmd_stats();
    printf("\",\n");
    printf("  \"blacklist\": \"");
    fflush(stdout);
    cmd_blacklist_list();
    printf("\"\n");
    printf("}\n");

    return 0;
}

static int handle_blacklist(int argc, char **argv)
{
    if (argc == 3 && strcmp(argv[2], "list") == 0)
        return cmd_blacklist_list();

    if (argc == 4 && strcmp(argv[2], "add") == 0)
        return cmd_blacklist_add(argv[3]);

    if (argc == 4 && strcmp(argv[2], "remove") == 0)
        return cmd_blacklist_remove(argv[3]);

    usage(argv[0]);
    return 1;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "status") == 0)
        return cmd_status();

    if (strcmp(argv[1], "stats") == 0)
        return cmd_stats();

    if (strcmp(argv[1], "report") == 0)
        return cmd_report();

    if (strcmp(argv[1], "blacklist") == 0)
        return handle_blacklist(argc, argv);

    usage(argv[0]);
    return 1;
}
