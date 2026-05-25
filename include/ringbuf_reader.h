#ifndef RINGBUF_READER_H
#define RINGBUF_READER_H

#include <bpf/libbpf.h>

struct ring_buffer *ringbuf_reader_create_proc(int map_fd);
struct ring_buffer *ringbuf_reader_create_tcp(int map_fd);

#endif
