#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include "cJSON.h"
#include "jsonParser.h"
#include "functions.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"


//! RICORDA DI VEDERE DIMENSIONE ARRAY LETTURA


int main(int argc, char *argv[]){
  
  int socketfd, clientfd;
  socklen_t clientAddressSize;
  struct sockaddr_in serverAddress, clientAddress;

  signal(SIGCHLD, SIG_IGN); // anti zombie handler

  // creazione socket
  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error creating socket");
    exit(-1);
  }
  printf("Server socket created successfully.\n");

  // definizio indirizzo server
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(SERVER_PORT); 
  serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

  // binding socket
  if(bind(socketfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
    perror("Error binding socket");
    close(socketfd);
    exit(-1);
  }

  // listen for incoming connections
  if(listen(socketfd, 1) == -1) {
    perror("Error listening on socket");
    close(socketfd);
    exit(-1);
  }

  printf("Server is listening on port %d\n", SERVER_PORT);

  

  // accept incoming connections
  while(1){
    clientAddressSize = sizeof(clientAddress);
    if((clientfd=accept(socketfd, (struct sockaddr*) &clientAddress, &clientAddressSize)) == -1) {
      perror("Error accepting connection");
      close(socketfd);
    }
    else {
      pid_t pid = fork();
      if(pid != 0) {
        // parent process
        close(clientfd); // close the client socket
      }
      else {
        // child process
        close(socketfd); // close the listening socket

        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        
        //lettura messaggio dal client e stampa
        char string[256]= {0};
        readLine(clientfd, string);
        // printf("Received from client n.%d: %s", clientfd, string);

        // parsing string to struct
        cJSON *jsonTicket = cJSON_Parse(string);
        Ticket ticket;
        parseJSONToTicket(jsonTicket, &ticket);

        // il server deve:
        // 1. assegnare un ID unico al ticket
        // 2. assegnare un agente al ticket
        // 3. inviare al client il numero del ticket
        // 4. salvare su file il ticket






  
        close(clientfd);
        exit(0); // terminate child process
      }
    }
  }
  
  return 0;
}