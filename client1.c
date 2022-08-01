#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

void sendRequest(int sockfd, char *sendBuffer, char *recvBuffer, char *message)
{
  int n;
  strcpy(sendBuffer, message);
  printf("Request: %s\n", sendBuffer);
  write(sockfd, sendBuffer, strlen(sendBuffer));

  n = read(sockfd, recvBuffer, 200 - 1);
  recvBuffer[n] = 0;
  printf("Response: %s\n", recvBuffer);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Incorrect format\n");
    return 0;
  }
  
  int sockfd = 0, n = 0;
  char recvBuffer[200];
  char sendBuffer[200];
  struct sockaddr_in dest_addr;

  memset(recvBuffer, '\0', sizeof(recvBuffer));

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Error : Could not create socket \n");
    return 1;
  }

  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(atoi(argv[2]));
  dest_addr.sin_addr.s_addr = inet_addr(argv[1]);

  if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
  {
    printf("\n Error : Connect Failed \n");
    return 1;
  }

  // Request 1
  sendRequest(sockfd, sendBuffer, recvBuffer, "0 100 3");
  // Request 2
  sendRequest(sockfd, sendBuffer, recvBuffer, "0 200 1");
  // Request 3
  sendRequest(sockfd, sendBuffer, recvBuffer, "0 098 2");
  //Request 4
  sendRequest(sockfd, sendBuffer, recvBuffer, "0 100 3");
  // Request 5
  sendRequest(sockfd, sendBuffer, recvBuffer, "0 200 1");
  //Request 6
  sendRequest(sockfd, sendBuffer, recvBuffer, "0 323 2");
  //Request 7
  sendRequest(sockfd, sendBuffer, recvBuffer, "1 0 0");

  return 0;
}
