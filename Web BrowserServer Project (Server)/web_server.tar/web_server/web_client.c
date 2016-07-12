#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>

char *buildQuery(char *host, char *page);

#define DEFAULT_HTTP_PORT 80

typedef struct url_s
{
  unsigned short usPort;   // in host byte order
  char          *szServer; // allocated by parse_url(), must be freed by the caller
  char          *szFile;   // allocated by parse_url(), must be freed by the caller
} url_t;


url_t parse_url(const char *szURL)
{
  url_t url;
  memset(&url, 0, sizeof(url));

	unsigned int urllen = strlen(szURL) + 1;
	url.szServer = (char*) malloc(urllen * sizeof(char));
	assert(NULL != url.szServer);
	url.szFile = (char*) malloc(urllen * sizeof(char));
	assert(NULL != url.szFile);

  char server[urllen];
  int result = sscanf(szURL, "http://%[^/]/%s", server, url.szFile);
  if (EOF == result)
  {
    fprintf(stderr, "Failed to parse URL: %s\n", strerror(errno));
    exit(1);
  }
  else if (1 == result)
  {
    url.szFile[0] = '\0';
  }
  else if (result < 1)
  {
    fprintf(stderr, "Error: %s is not a valid HTTP request\n", szURL);
    exit(1);
  }

  result = sscanf(server, "%[^:]:%hu", url.szServer, &url.usPort);
  if (EOF == result)
  {
    fprintf(stderr, "Failed to parse URL: %s\n", strerror(errno));
    exit(1);
  }
  else if (1 == result)
  {
    url.usPort = DEFAULT_HTTP_PORT;
  }
  else if (result < 1)
  {
    fprintf(stderr, "Error: %s is not a valid HTTP request\n", szURL);
    exit(1);
  }

	assert(NULL != url.szServer);
	assert(NULL != url.szFile);
	assert(url.usPort > 0);
  return url;
}


int main(int argc, char **argv)
{
  if (argc < 2)
  {
    fprintf(stderr,
            "Usage: http_client URL\n");
    exit(1);
  }

  // Parse the URL
  url_t url = parse_url(argv[1]);
  fprintf(stderr, "Server: %s:%hu\nFile: /%s\n",
          url.szServer, url.usPort, url.szFile);

struct sockaddr_in *svr_addr;
  int sock;
  int tmpres;
  char *ip;
  char *get;
  char buf[BUFSIZ];
  struct hostent *hent;

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {perror("Cannot create socket"); exit(1);}

  ip = (char *)malloc(16);
  memset(ip, 0, 16);

  // Create socket
  if ((hent = gethostbyname(url.szServer)) == NULL) {perror("Can't get IP"); exit (1);} // Get host name
  if (inet_ntop(AF_INET, (void *)hent->h_addr_list[0], ip, 15) == NULL) {perror("Cannot resolve host"); exit(1);}


  fprintf(stderr, "IP: %s\n", ip); // Debug print IP
  svr_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
  svr_addr->sin_family = AF_INET;
  tmpres = inet_pton(AF_INET, ip, (void *)(&(svr_addr->sin_addr.s_addr))); //
  if( tmpres < 0)  
  {
    perror("Can't set svr_addr->sin_addr.s_addr");
    exit(1);
  }else if(tmpres == 0)
  {
    fprintf(stderr, "%s is not a valid IP address\n", ip);
    exit(1);
  }
  svr_addr->sin_port = htons(url.usPort);
 
  if(connect(sock, (struct sockaddr *)svr_addr, sizeof(struct sockaddr)) < 0){
    perror("Could not connect");
    exit(1);
  }
  get = buildQuery(url.szServer, url.szFile);
  fprintf(stderr, "\n%s", get); // 1 pt. - creating a correct HTTP GET request and printing it to stderr
 
  //Send the query to the server
  int sent = 0;
  while(sent < strlen(get)) //1.5 pts. - sending the HTTP request to the server
  {
    tmpres = send(sock, get+sent, strlen(get)-sent, 0);
    if(tmpres == -1){
      perror("Can't send query");
      exit(1);
    }
    sent += tmpres;
  }
  
  // Receive response
  
  memset(buf, 0, sizeof(buf));
    int temp;
    int htmlStart = 0;
    const char * pointer;
    int i = 0, k;
    int size = 0;
    while((temp = recv(sock, buf, BUFSIZ-1, 0)) > 0) {
      if (!htmlStart) {
            pointer = buf;
            char *htmlContent = strstr(buf, "\r\n\r\n");
            htmlContent =  htmlContent + 4;
            htmlStart = 1;
            while ((void *)pointer != (void *)htmlContent ) {
		  pointer++;
                  i++;
            }
            size = size + temp - i;
            for (k = 0; k < temp - i; k ++) {
              fprintf(stdout,"%c", htmlContent[k]);
            }
      } else {
        size = temp+size;
        for (k = 0; k < temp; k ++) {
              fprintf(stdout,"%c", buf[k]);
        }
    }
    memset(buf, 0, temp+1);
    }
  free(get);
  free(svr_addr);
  free(ip);
  close(sock);
  return 0;
}

char* buildQuery(char *host, char *page)
{
  char *query;
  char *userAgent = "WEB_CLIENT SYANG";
  char *connection = "close";
  char *acceptLanguage = "en";
  char *tpl = "GET /%s HTTP/1.1\r\nHost: %s\r\nConnection: %s\r\nUser-Agent: %s\r\nAccept-language: %s\r\n\r\n";
  query = (char *)malloc(strlen(host)+strlen(page)+strlen(connection)+strlen(userAgent)+strlen(acceptLanguage)+strlen(tpl)-7);
  sprintf(query, tpl, page, host, connection, userAgent, acceptLanguage);
  return query;
}
