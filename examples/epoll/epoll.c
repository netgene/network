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

static int shark_socket_non_blocking(int sfd)
{
	int flags, s;

	flags = fcntl(sfd, F_GETFL, 0);
	if(flags == -1) {
		perror ("fcntl");
		return -1;
	}

	s = fcntl(sfd, F_SETFL, flags | O_NONBLOCK);
	if(s == -1) {
		perror ("fcntl");
		return -1;
	}

	return 0;
}

static int shark_socket_create(char *port)
{
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int s, sfd;

	memset (&hints, 0, sizeof (struct addrinfo));
	hints.ai_family = AF_UNSPEC;     /* Return IPv4 and IPv6 choices */
	hints.ai_socktype = SOCK_STREAM; /* We want a TCP socket */
	hints.ai_flags = AI_PASSIVE;     /* All interfaces */

	s = getaddrinfo (NULL, port, &hints, &result);
	if (s != 0) {
      fprintf (stderr, "getaddrinfo: %s\n", gai_strerror (s));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
      sfd = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1)
        continue;

      s = bind (sfd, rp->ai_addr, rp->ai_addrlen);
      if (s == 0) {
          /* We managed to bind successfully! */
          break;
        }

        printf("close fd:%d\n", sfd);
		close(sfd);
    }

    if (rp == NULL) {
      fprintf (stderr, "Could not bind\n");
      return -1;
    }
    freeaddrinfo(result);

	s = shark_socket_non_blocking(sfd);
	if(s == -1)
		abort ();

	s = listen (sfd, SOMAXCONN);
	if (s == -1) {
      perror ("listen");
      abort ();
    }

	return sfd;	
}

static int shark_epoll_add(struct _shark *sk, int fd)
{
	struct _epoll *epoll = &(sk->epoll);
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
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
	event.events = EPOLLIN | EPOLLET;
	s = epoll_ctl(epoll->efd, EPOLL_CTL_ADD, sk->sfd, &event);
	if (s == -1) {
      perror ("epoll_ctl");
      abort ();
    }

	epoll->events = calloc(MAXEVENTS, sizeof(struct epoll_event));
	
	return 0;
}

static int shark_socket_accept(struct _shark *sk)
{
	int sfd = sk->sfd;
	struct sockaddr in_addr;
	socklen_t in_len;
	int infd;
	in_len = sizeof(in_addr);
	infd = accept(sfd, &in_addr, &in_len);
	if(infd == -1) {
		if((errno == EAGAIN || errno == EWOULDBLOCK)) {
			//
		} else {
			perror("accept");
			return -1;
		}
	}

	shark_socket_non_blocking(infd);
	shark_epoll_add(sk, infd);

	return 0;
}

static int shark_recv_data(struct _shark *sk, struct epoll_event *event)
{
	int done = 0;
	int s;

	while(1) {
		ssize_t count;
		char buf[512];

		count = read(event->data.fd, buf, sizeof buf);
		if(count == -1) {
			/* If errno == EAGAIN, that means we have read all
			data. So go back to the main loop. */
			if(errno != EAGAIN) {
				perror ("read");
				done = 1;
			}
			break;
		} else if(count == 0) {
			/* End of file. The remote has closed the
				connection. */
			done = 1;
			break;
		}
		
		/* Write the buffer to standard output */
		s = write (event->data.fd, buf, count);
		if(s == -1) {
			perror ("write");
			abort ();
		}
		if(done) {
			printf ("Closed connection on descriptor %d\n",
				event->data.fd);
			/* Closing the descriptor will make epoll remove it
				from the set of descriptors which are monitored. */
			close (event->data.fd);
		}
	}

	return 0;
}

static int shark_epoll_deal(struct _shark *sk)
{
	struct _epoll *epoll = &sk->epoll;
	struct epoll_event *events = epoll->events;
	int sfd = sk->sfd;
	int n, i, infd;

	n = epoll_wait(epoll->efd, epoll->events, MAXEVENTS, -1);
	for(i = 0; i < n; i++) {
		if((events[i].events & EPOLLERR) || 
			(events[i].events & EPOLLHUP) ||
			(!(events[i].events & EPOLLIN)) ) {
			fprintf(stderr, "epoll error[%d]\n", events[i].data.fd);
			close(events[i].data.fd);
			continue;
		} else if(sfd == events[i].data.fd) {
			shark_socket_accept(sk);
		} else {
			shark_recv_data(sk, &events[i]);
		}
	}

	return 0;
}

static int shark_epoll_close(struct _epoll *epoll)
{
	free(epoll->events);

	return 0;
}

static int shark_socket_close(struct _shark *sk)
{
	shark_epoll_close(&sk->epoll);
	close(sk->sfd);

	return 0;
}

int main(int argc, char **argv)
{
	if(argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(0);
	}

	struct _shark sk;
	memset(&sk, 0x00, sizeof(struct _shark));

	snprintf(sk.port, 20, "%s", argv[1]);

	sk.sfd = shark_socket_create(sk.port);	
	shark_epoll_create(&sk);
	while(1) {
		if(shark_epoll_deal(&sk) != 0) {
			break;
		}
	}
	shark_socket_close(&sk);

	return 0;
}
