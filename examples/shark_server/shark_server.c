#include "shark_socket.h"
#include <stdio.h>

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
