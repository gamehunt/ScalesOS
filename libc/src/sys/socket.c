#include "sys/socket.h"
#include "errno.h"
#include "sys/syscall.h"

int socket(int domain, int type, int protocol) {
	__return_errno(__sys_socket(domain, type, protocol));
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	__return_errno(__sys_connect(sockfd, (uint32_t) addr, addrlen));
}

int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
	__return_errno(__sys_bind(sockfd, (uint32_t) addr, addrlen));
}

int listen(int sockfd, int backlog) {
	__return_errno(__sys_listen(sockfd, backlog));
}

int accept(int sockfd, struct sockaddr* addr, socklen_t* addrlen) {
	__return_errno(__sys_accept(sockfd, (uint32_t) addr, (uint32_t) addrlen));
}
