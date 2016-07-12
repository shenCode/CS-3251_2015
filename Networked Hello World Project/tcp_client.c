#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <arpa/inet.h>


// Globals
const char *g_szMessage;
const char *g_szServer;
unsigned short g_usPort;


// Function Prototypes
void parse_args(int argc, char **argv);


// Function Implementations

int main(int argc, char **argv)
{
  /* Anything that doesn't == 1 means failure. */

  parse_args(argc, argv);
  printf("Sending message \"%s\" to %s:%hu\n",
         g_szMessage, g_szServer, g_usPort);
  
  int sock; // file descriptor
  struct sockaddr_in svr_addr; // structure containing server address info

  /* Create the socket */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    perror("tcp_client: socket failed to create");
    exit(1);
  }
  printf("Socket created.\n");

  /* Set the properties */
  svr_addr.sin_family = AF_INET;
  svr_addr.sin_port = htons(g_usPort);
  inet_aton(g_szServer, &svr_addr.sin_addr); // Convert input

  /* Connect */
  if (connect(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr)) < 0) {
    perror("tcp_client: connect error");
    close(sock);
    exit(1);
  }
  printf("Connected.\n");

  /* Send */
  if (send(sock, g_szMessage, strlen(g_szMessage), 0) < 0) { 
    perror("tcp_client: send error");
    close(sock);
    exit(1);
  }
  printf("Message has been sent.\n");
  close(sock);
  return 0;
}


void parse_args(int argc, char **argv)
{
  if (argc < 4)
  {
    fprintf(stderr,
            "Usage: tcp_client SERVER PORT \"MESSAGE\"\n");
    exit(1);
  }

  errno = 0;
  char *endptr = NULL;
  unsigned long ulPort = strtoul(argv[2], &endptr, 10);

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
            argv[2], strerror(errno));
    abort();
  }
  g_usPort = ulPort;

  g_szServer = argv[1];
  g_szMessage = argv[3];
}
