#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define DEFAULT_PORT 80
#define EOL "\r\n"
#define ROOT "./home"

typedef struct sockaddr_in sockaddr;

void parse_args(int argc, char** argv);
void handle_connection(int clt_sock, sockaddr *clt_addr);
int recv_ln(int clt_sock, unsigned char *req);
int sendstr(int clt_sock, unsigned char *buf);
void get_file_info(int file_desc);
char* get_time();
char* get_mtime(struct timespec);
char* get_ext(unsigned char *filename);

unsigned short g_usPort;
sockaddr svr_addr, clt_addr;
struct stat file_macro;
int svr_sock, clt_sock;


int main(int argc, char** argv) {
	parse_args(argc, argv);
	fprintf(stderr, "Server starting on Port: %d\n", g_usPort);

	if ((svr_sock = socket(AF_INET, SOCK_STREAM, 0))  < 0) {
		fprintf(stderr, "web_server: Error opening port");
		exit(1);
	}

	svr_addr.sin_family = AF_INET;
	svr_addr.sin_port = htons(g_usPort);
	svr_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(svr_sock, (const struct sockaddr*) &svr_addr, sizeof(svr_addr)) < 0) {
		fprintf(stderr, "web_server: Error binding.");
		exit(1);
	}
	if (listen(svr_sock, 5) < 0) {
		fprintf(stderr, "web_server: Error listening");
		exit(1);
	}
	socklen_t sin_size;
	sin_size = sizeof(sockaddr);
	
	while (1) {
		if ((clt_sock = accept(svr_sock, (struct sockaddr*) &clt_addr, &sin_size)) < 0) {
			fprintf(stderr, "web_server: Error accepting");
		}
		handle_connection(clt_sock, &clt_addr);
	}
}

void handle_connection(int client_socket, sockaddr *client_addr) {
	unsigned char *temp, request[1000], file[1000];
	char* cur_time, *mtim, *file_ext;
	char buf_0[20];
	int buf_len, file_desc;

	buf_len = recv_ln(client_socket, request);

	temp = (unsigned char *)strstr((const char*) request, (const char *)" HTTP/");
	if (temp == 0) {
		fprintf(stderr, "Responding with \"400 Bad Request\".\n");
			sendstr(client_socket, (unsigned char*)"HTTP/1.1 400 Bad Request\r\nServer: Web_Server\r\n");
			sendstr(client_socket, (unsigned char*)"Date: ");
			cur_time = get_time();
			sendstr(client_socket, (unsigned char*) strcat(cur_time, "\r\n\r\n"));
			sendstr(client_socket, (unsigned char*) "Connection: close\r\n");
			sendstr(client_socket, (unsigned char*)"<html><head><title>400 Bad Request</title></head><body><h1>400 Bad Request</h1></body></html>\r\n");
			return;
		}
	*temp = 0;
	if (strncmp((const char*) request, (const char*) "GET", 3) == 0) {
		temp = request + 4;
	} else {
		fprintf(stderr, "Responding with \"501 Not Implemented\".\n");
		sendstr(client_socket, (unsigned char*)"HTTP/1.1 501 Not Implemented\r\nServer: Web_Server\r\n");
		sendstr(client_socket, (unsigned char*)"Date: ");
		cur_time = get_time();
		sendstr(client_socket, (unsigned char*) strcat(cur_time, "\r\n\r\n"));
		sendstr(client_socket, (unsigned char*) "Connection: close\r\n");
		sendstr(client_socket, (unsigned char*)"<html><head><title>501 Not Implemented</title></head><body><h1>501 Not Implemented</h1></body></html>\r\n");
		return;
	}
	if (*(temp + strlen((const char*) temp) - 1) == '/') {
		strcat((char *) temp, "index.html");
	}
	file_ext = get_ext(temp);
	strcpy((char *) file, ROOT);
	strcat((char *) file, (const char *) temp);
	file_desc = open((const char *)file, O_RDONLY, 0);
	get_file_info(file_desc);
	if (file_desc < 0) {
		sendstr(client_socket, (unsigned char*) "HTTP/1.1 404 Not Found\r\nServer: Web_Server\r\n");
		sendstr(client_socket, (unsigned char*) "Date: ");
		cur_time = get_time();
		sendstr(client_socket, (unsigned char*) strcat(cur_time, "\r\n\r\n"));
		sendstr(client_socket, (unsigned char*) "Connection: close\r\n");
		sendstr(client_socket, (unsigned char*) "<html><head><title>404 Not Found</title></head><body><h1>404 Not Found</h1></body></html>\r\n");
	} else if ((buf_len = (file_macro.st_size)) == -1) {
		fprintf(stderr, "Error fetching file status.\n");
		sendstr(client_socket, (unsigned char*) "HTTP/1.1 500 Internal Server Error\r\nServer: Web_Server\r\n");
		sendstr(client_socket, (unsigned char*) "Date: ");
		cur_time = get_time();
		sendstr(client_socket, (unsigned char*) strcat(cur_time, "\r\n\r\n"));
		sendstr(client_socket, (unsigned char*) "Connection: close\r\n");
		sendstr(client_socket, (unsigned char*) "<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>\r\n");
	} else if ((temp = (unsigned char*) malloc(buf_len)) == 0) {
		fprintf(stderr, "Error allocating memory for file buffer.");
		sendstr(client_socket, (unsigned char*) "HTTP/1.1 500 Internal Server Error\r\nServer: Web_Server\r\n");
		sendstr(client_socket, (unsigned char*) "Date: ");
		cur_time = get_time();
		sendstr(client_socket, (unsigned char*) strcat(cur_time, "\r\n\r\n"));
		sendstr(client_socket, (unsigned char*) "Connection: close\r\n");
		sendstr(client_socket, (unsigned char*) "<html><head><title>500 Internal Server Error</title></head><body><h1>500 Internal Server Error</h1></body></html>\r\n");
	} else {
		sendstr(client_socket, (unsigned char*) "HTTP/1.1 200 OK\r\nServer: Web_Server\r\n");
		sendstr(client_socket, (unsigned char*) "Date: ");
		cur_time = get_time();
		sendstr(client_socket, (unsigned char*) strcat(cur_time, "\r\n"));
		sendstr(client_socket, (unsigned char*) "Last-Modified: ");
		mtim = get_mtime(file_macro.st_mtim);
		sendstr(client_socket, (unsigned char*) strcat(mtim, "\r\n"));
		sendstr(client_socket, (unsigned char*) "Connection: close\r\n");
		sendstr(client_socket, (unsigned char*) "Content-Length: ");
		sprintf(buf_0, "%d", (int) file_macro.st_size);
		sendstr(client_socket, (unsigned char*) strcat(buf_0, "\r\n"));
		sendstr(client_socket, (unsigned char*) "Content-Type: "); // jpg -> image/jpeg
		sendstr(client_socket, (unsigned char*) file_ext);
		sendstr(client_socket, (unsigned char*) "\r\n\r\n");
		read(file_desc, temp, buf_len); // Checks if read gets entire file
		send(client_socket, temp, buf_len, 0); // Loop
		free(temp);
		free(mtim);
		free(cur_time);
	}
	close(file_desc);
	shutdown(client_socket, SHUT_RDWR);
}

int recv_ln(int clt_sock, unsigned char *req) {
	unsigned char *tmp;
	int eol = 0;

	tmp = req;
	while (recv(clt_sock, tmp, 1, 0) == 1) {
		if ((*tmp == EOL[eol])) {
			eol++;
			if ((eol == 2)) {
				*(tmp - 1) = 0;
			}
			return strlen((const char *) req);
		} else {
			eol = 0;
		}
		tmp++;
	}
	return 0;
}

int sendstr(int clt_sock, unsigned char *buf) {
	int buf_sz, sent_sz;
	buf_sz = strlen((const char *) buf);
	while (buf_sz > 0) {
		if ((sent_sz = send(clt_sock, buf, buf_sz, 0)) < 0) {
			return -1;
		}
		buf_sz -= sent_sz;
		buf += sent_sz;
	}
	return 1;
}
void get_file_info(int file_desc) {
	fstat(file_desc, &file_macro);
}

void parse_args(int argc, char **argv) {
	if (argc < 2) {
		g_usPort = DEFAULT_PORT;
 		} else {
  		errno = 0;
  		char *endptr = NULL;
  		unsigned long ulPort = strtoul(argv[1], &endptr, 10);
  		if (0 == errno) {
			if ('\0' != endptr[0]) {
		  		errno = EINVAL;
			} else if (ulPort > USHRT_MAX) {
	  			errno = ERANGE;
			}
  		}
  		if (0 != errno) {
			fprintf(stderr, "Failed to parse port number \"%s\": %s\n",
			argv[1], strerror(errno));
			abort();
  		}
  		g_usPort = ulPort;
	}
}

char* get_time() {
	time_t rawtime;
	struct tm* time_info;
	char* buf = (char*) malloc(80);
	time(&rawtime);
	time_info = gmtime(&rawtime);
	strftime(buf, 80, "%a, %d %b %Y %T %Z", time_info);
	return buf;
}

char* get_mtime(struct timespec t_spec) {
	time_t rawtime = t_spec.tv_sec;
	struct tm* time_info;
	char* buf = (char*) malloc(80);
	time_info = gmtime(&rawtime);
	strftime(buf, 80, "%a, %d %b %Y %T %Z", time_info);
	return buf;
}

char* get_ext(unsigned char *filename) {
	char *dot = strrchr((const char*) filename, '.');
	if(!dot) {
		return "raw";
	}
	return dot + 1;
}