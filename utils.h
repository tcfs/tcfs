#ifndef _UTILS_H
#define _UTILS_H

#include <stdint.h>

int client_connect(const char *addr, uint16_t port);
int readn(int fd, char *ptr, int nbytes);
int writen(int fd, char *ptr, int nbytes);
ssize_t writevn(int fd, struct iovec *vector, int count, size_t n);

uint32_t buf_get_uint32(const char *buf);

#endif
