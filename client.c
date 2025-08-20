#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "ticket.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"


//TODO: struttura per ticket

// read until null terminator
int readLine(int fd, char *string) {
  int bytesRead;

  do{
    bytesRead = read(fd, string, 1);
  }
  while(bytesRead>0 && *string++ != '\0');

  return bytesRead;
}

int main(int argc, char *argv[]){
  
  int clientfd, resultCode;
  struct sockaddr_in serverAddress;

  // create socket
  clientfd = socket(AF_INET, SOCK_STREAM, 0);

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(SERVER_PORT);
  serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

  // connection
  do
  {
    resultCode = connect(clientfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if(resultCode == -1) {
      printf("Error connecting to server, trying again in 1 second\n");
      sleep(1);
    }
  } while (resultCode == -1);

  printf("Connected to server at %s:%d\n", SERVER_ADDRESS, SERVER_PORT);

  //TODO: scrittura ticket (IN JSON)
  char *message = "Il Bertini è in vacanza\n";
  write(clientfd, message, strlen(message)+1); // +1 to include \0
  printf("Sent to server: Hello from client!\n");  

  // close socket
  close(clientfd);
  printf("Connection closed.\n");


  exit(0);
}