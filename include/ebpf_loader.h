#ifndef EBPF_LOADER_H
#define EBPF_LOADER_H

int ebpf_loader_start(const char *object_path);
int ebpf_loader_poll(int timeout_ms);
void ebpf_loader_stop(void);

#endif
