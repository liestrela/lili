#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#define DEFAULT_ADDR INADDR_ANY
#define DEFAULT_PORT 8080
#define DEFAULT_MAXCONN -1

int
init_socket(void)
{
	int socket_in, socket_opt;
	struct sockaddr_in socket_addr;

	/* create socket */
	socket_in = socket(AF_INET, SOCK_STREAM, 0);

	/* enable address reuse for socket */
	socket_opt = 1;
	if (setsockopt(socket_in, SOL_SOCKET, SO_REUSEADDR, &socket_opt,
	               sizeof(socket_opt))<0) {
		err(EXIT_FAILURE, "socket configuration");
	}

	/* bind socket address */
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(DEFAULT_PORT);
	socket_addr.sin_addr.s_addr = DEFAULT_ADDR;
	
	if (bind(socket_in, (struct sockaddr *)&socket_addr,
	         sizeof(socket_addr))<0) {
		char addr_str[INET_ADDRSTRLEN];

		inet_ntop(AF_INET, &socket_addr.sin_addr, addr_str, 
		          INET_ADDRSTRLEN);

		err(EXIT_FAILURE, "couldn't bind address %s:%u to socket",
		    addr_str, ntohs(socket_addr.sin_port));
	}

	/* listen passively on socket */
	if (listen(socket_in, DEFAULT_MAXCONN)<0)
		err(EXIT_FAILURE, "couldn't set socket as passive");
	
	return socket_in;
}

int
main(void)
{
	int socket_in;

	/* init incoming socket */
	socket_in = init_socket();

	/* close incoming socket */
	close(socket_in);

	return EXIT_SUCCESS;
}
