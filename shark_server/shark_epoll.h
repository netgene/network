#ifndef shark_epoll_h
#define shark_epoll_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

#define MAXEVENTS 64

struct _epoll{
	int efd;
	struct epoll_event *events;
};

struct _shark {
	char port[20];
	int sfd;
	struct _epoll epoll;
};

static int shark_epoll_add(struct _shark *sk, int fd)
{
	struct _epoll *epoll = &(sk->epoll);
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN; // default LT
	epoll_ctl(epoll->efd, EPOLL_CTL_ADD, fd, &event);
	
	return 0;
}

static int shark_epoll_create(struct _shark *sk)
{
	struct _epoll *epoll = &(sk->epoll);
	struct epoll_event event;
	int s;

	epoll->efd = epoll_create1(0);
	event.data.fd = sk->sfd;
	event.events = EPOLLIN; // default LT
	s = epoll_ctl(epoll->efd, EPOLL_CTL_ADD, sk->sfd, &event);
	if (s == -1) {
      perror ("epoll_ctl");
      abort ();
    }

	epoll->events = calloc(MAXEVENTS, sizeof(struct epoll_event));
	
	return 0;
}

static int shark_epoll_close(struct _epoll *epoll)
{
	free(epoll->events);

	return 0;
}

#endif