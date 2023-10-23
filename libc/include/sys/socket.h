#ifndef __SYS_SOCKET_H
#define __SYS_SOCKET_H

#include <stddef.h>
#include <stdint.h>

#define AF_LOCAL 0
#define AF_UNIX  AF_LOCAL

#define SOCK_NONBLOCK (1 << 0)
#define SOCK_CLOEXEC  (1 << 1)

#define SOCK_STREAM     (SOCK_CLOEXEC << 1)
#define SOCK_DGRAM      (SOCK_CLOEXEC << 2)
#define SOCK_SEQPACKET  (SOCK_CLOEXEC << 3)
#define SOCK_RAW        (SOCK_CLOEXEC << 4)
#define SOCK_RDM        (SOCK_CLOEXEC << 5)

typedef uint16_t sa_family_t;
typedef size_t   socklen_t;

struct sockaddr {
    sa_family_t sa_family;
    char        sa_data[14];
};

int socket(int domain, int type, int protocol);
int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockfd, int backlog);
int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen);

#endif
