#ifndef shark_socket_h
#define shark_socket_h

#include "shark_epoll.h"
#include "shark_fix_buffer.h"

int shark_socket_non_blocking(int sfd);
int shark_socket_create(struct _shark *sk);
int shark_socket_accept(struct _shark *sk);
int shark_epoll_deal(struct _shark *sk);
int shark_recv_data(struct _shark *sk, struct epoll_event *event);
int shark_socket_deal(struct _shark *sk);
int shark_socket_close(struct _shark *sk);

#endif