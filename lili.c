#include <err.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <sys/socket.h>

#define DEFAULT_ADDR INADDR_ANY
#define DEFAULT_PORT 8080
#define DEFAULT_MAXCONN -1 /* system max */

#define MAX_REQUEST_LEN 4096 /* 4 kB */

static const char *http_template =
	"HTTP/1.0 %d %s\r\n"
	"Server: %s\r\n"
	"Content-Type: text/html\r\n"
	"Content-Length: %d\r\n"
	"\r\n%s";

static const char *usage_msg =
	"usage: %s [www-root] (optional arguments)\n"
	"optional arguments:\n"
	"--port p: set server listen port to p (default is %d)\n"
	"--addr a: set server listen IPv4 address to a (default is "
	"ALL)\n"
	"--maxconn n: set connection queue max size to n (default "
	"is the system max)\n";

static char *www_root;
static const char *srv_version = "lili/0.01";
static in_addr_t srv_addr = DEFAULT_ADDR;
static unsigned short srv_port = DEFAULT_PORT;
static int srv_maxconn = DEFAULT_MAXCONN;
static int brk = 0;

#ifdef DEBUG
static int debug = 1;
#else
static int debug = 0;
#endif

/* print formatted debug message */
void
dprintf(const char *fmt, ...)
{
	va_list va;

	va_start(va, fmt);
	printf("debug: ");
	vprintf(fmt, va);
	printf("\n");
	va_end(va);
}

void
usage(const char *execname)
{
	printf(usage_msg, execname, DEFAULT_PORT);
	exit(EXIT_SUCCESS);
}

int
init_socket(void)
{
	int socket_in, socket_opt;
	struct sockaddr_in socket_addr;
	char addr_str[INET_ADDRSTRLEN];

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
	socket_addr.sin_port = htons(srv_port);
	socket_addr.sin_addr.s_addr = srv_addr;

	inet_ntop(AF_INET, &socket_addr.sin_addr, addr_str,
	          INET_ADDRSTRLEN);

	if (bind(socket_in, (struct sockaddr *)&socket_addr,
	         sizeof(socket_addr))<0) {
		err(EXIT_FAILURE, "couldn't bind address %s:%u to socket",
		    addr_str, ntohs(socket_addr.sin_port));
	}

	if (debug) {
		dprintf("address %s:%u binded to socket (%d)\n", addr_str,
		       ntohs(socket_addr.sin_port), socket_in);
	}

	/* listen passively on socket */
	if (listen(socket_in, srv_maxconn)<0)
		err(EXIT_FAILURE, "couldn't set socket as passive");

	printf("Server listening on %s:%u\n", addr_str,
	       ntohs(socket_addr.sin_port));

	return socket_in;
}

void
parse_args(int argc, char **argv)
{
	if (argc<2) usage(argv[0]);

	www_root = argv[1];

	/* check if www_root exists (should sanitize here) */
	if (access(www_root, F_OK)<0)
		err(EXIT_FAILURE, "couldn't open www-root directory");

	/* parse the remaining arguments */
	for (int i=2; i<argc; i++) {
		if (strcmp(argv[i], "--port")==0) {
			if (i+1>=argc) usage(argv[0]);

			if ((srv_port = atoi(argv[i+1]))==0) {
				errx(EXIT_FAILURE, "invalid given port number");
			}
		}

		if (strcmp(argv[i], "--addr")==0) {
			if (i+1>=argc) usage(argv[0]);

			/* convert ipv4 from text to binary form */
			if ((inet_pton(AF_INET, argv[i+1], &srv_addr))==0) {
				errx(EXIT_FAILURE, "invalid given IP address");
			}
		}

		if (strcmp(argv[i], "--maxconn")==0) {
			if (i+1>argc) usage(argv[0]);

			if ((srv_maxconn = atoi(argv[i+1]))==0) {
				errx(EXIT_FAILURE, "invalid given maxconn number");
			}
		}
	}
}

/* minimal response (mostly for errors) */
void
small_response(int sock, int code, const char *msg, const char *body)
{
	char res_buf[8096];
	int res_len;

	res_len = snprintf(res_buf, sizeof(res_buf), http_template,
	                   code, msg, srv_version, strlen(body), body);

	if (send(sock, res_buf, res_len, 0)<0) {
		warn("couldn't send response");
	}
}

void
srv_poll(int socket_in)
{
	int req_sock, req_len;
	char req_buf[8096];
	socklen_t sock_in_len = sizeof(srv_addr);

	/* accept request */
	if ((req_sock = accept(socket_in, (struct sockaddr *)&srv_addr,
	                       &sock_in_len))<0) {
		warn("couldn't establish connection");
	}

	/* receive data from request */
	if ((req_len = recv(req_sock, req_buf, sizeof(req_buf), 0))<0) {
		warn("couldn't receive data from request");
	}

	if (debug) {
		dprintf("%d bytes received from request (%d)", req_len,
		        req_sock);
	}

	/* 413 - Payload Too Large */
	if (req_len>MAX_REQUEST_LEN) {
		if (debug) dprintf("payload too large");

		small_response(req_sock, 413, "Payload Too Large",
		               "<h1>The request entity was larger than the "
		               "limits defined by the server</h1>");
	}
}

int
main(int argc, char **argv)
{
	int socket_in;

	parse_args(argc, argv);

	/* init incoming socket */
	socket_in = init_socket();

	/* polling incoming requests */
	while (!brk) srv_poll(socket_in);

	/* close incoming socket */
	close(socket_in);

	return EXIT_SUCCESS;
}
