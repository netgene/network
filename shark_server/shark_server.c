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

	if(shark_socket_create(&sk) != 0) {
		return -1;
	}
	
	while(1) {
		if(shark_socket_deal(&sk) != 0) {
			break;
		}
	}
	shark_socket_close(&sk);

	return 0;
}
