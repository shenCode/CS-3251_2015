#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>


// Globals
unsigned short g_usPort;


// Function Prototypes
void parse_args(int argc, char **argv);


// Function Implementations

int main(int argc, char **argv)
{
  parse_args(argc, argv);
  printf("Starting TCP server on port: %hu\n", g_usPort);

  /* Anything that doesn't == 1 means failure. */

  /* Variables */
  int sock, new_sock; // file descriptors
  struct sockaddr_in addr; // structure containing server address info
  char buff[4096]; // buffer to receive message
  
  /* Create socket */
  sock = socket(AF_INET, SOCK_STREAM, 0); // SOCK_STREAM for TCP, SOCK_DGRAM for UDP
  if (sock < 0) {
    perror("tcp_server failed to create socket.");
    exit(1);
  }
  printf("Socket created.\n");

  /* Set properties */
  addr.sin_family = AF_INET; // This is always AF_INET
  addr.sin_addr.s_addr = htons(INADDR_ANY); // IP Address
  addr.sin_port = htons(g_usPort); // Port

  /* Bind */
  if (bind(sock, (struct sockaddr *) &addr, sizeof(addr))) {
    perror("tcp_server bind error");
    exit(1);
  }
  printf("Binded.\n");

  /* Listen */
  if (listen(sock, 5) < 0) {
    perror("tcp_server listen error");
    exit(1);
  }
  printf("Listening\n");

  /* Accept */
  while (1) {
    new_sock = accept(sock, (struct sockaddr *) 0, 0); // Accept new socket
    if (new_sock < 0) {
      perror("tcp_server: accept error");
      exit(1);
    }
    printf("Accepted.\n");
    memset(buff, 0, sizeof(buff));
    if (recv(new_sock, buff, sizeof(buff), 0)<0) { // Receives message
	perror("tcp_server: receive error");
        exit(1);
    }
    printf("Message received.\n");
    printf("%s\n", buff);
    close(new_sock);

  }


  return 0;
}


void parse_args(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr,
            "tcp_server: missing port operand\nUsage: tcp_server PORT\n");
    exit(1);
  }

  errno = 0;
  char *endptr = NULL;
  unsigned long ulPort = strtoul(argv[1], &endptr, 10);

  if (0 == errno)
  {
    // If no other error, check for invalid input and range
    if ('\0' != endptr[0])
      errno = EINVAL;
    else if (ulPort > USHRT_MAX)
      errno = ERANGE;
  }
  if (0 != errno)
  {
    // Report any errors and abort
    fprintf(stderr, "Failed to parse port number \"%s\": %s\n",
            argv[1], strerror(errno));
    abort();
  }
  g_usPort = ulPort;
}
