#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "../lib/cJSON/cJSON.h"
#include "../include/jsonParser.h"
#include "../include/functions.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"


//! RICORDA DI VEDERE DIMENSIONE ARRAY LETTURA


void loadTicketList(char **shmTicketList){
  FILE *file = fopen("./data/ticketList.json", "r");
  if(file != NULL){
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    if(fileSize == 0) return;

    // ticketList = malloc(fileSize + 1);
    *shmTicketList = mmap(NULL, fileSize+1, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    (*shmTicketList)[fileSize] = '\0';

    fread(*shmTicketList, 1, fileSize, file);

    
  }
  fclose(file);
}

// void saveTicketList(cJSON *shmTicketList){
//   printf("Saving ticket list to file...\n");
//   FILE *file = fopen("./data/ticketList.json", "w");

//   fwrite(shmTicketList, sizeof(char), strlen(shmTicketList), file);
//   fclose(file);
// }

// int getNextTicketId(cJSON *ticketList){
//   int maxId = 0;
//   cJSON *ticket = NULL;

//   cJSON_ArrayForEach(ticket, ticketList){
//     int currentId = cJSON_GetObjectItem(ticket, "id")->valueint;
//     if(currentId > maxId) maxId = currentId;
//   }

//   return maxId+1;
// }

int main(int argc, char *argv[]){
  
  int socketfd, clientfd;
  socklen_t clientAddressSize;
  struct sockaddr_in serverAddress, clientAddress;
  char *shmTicketList = NULL;

  loadTicketList(&shmTicketList); // loading existing tickets

  printf("Loaded ticket list: %s\n", shmTicketList);

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
        // //* CREAZIONE TICKET
        // cJSON *jsonTicket = cJSON_Parse(string);
        // cJSON_ReplaceItemInObject(jsonTicket, "id", cJSON_CreateNumber(getNextTicketId(ticketList)));
        // printf("Ticket ID assigned: number %d.\n", cJSON_GetObjectItem(jsonTicket, "id")->valueint);
        
        // cJSON_AddItemToArray(ticketList, jsonTicket);

        // saveTicketList(ticketList);

        // printf("Ticket saved.\n");




        // il server deve:
        // 1. assegnare un ID unico al ticket
        // 2. assegnare un agente al ticket
        // 3. inviare al client il numero del ticket
        // 4. salvare su file il ticket






  
        close(clientfd);
        printf("Connection closed.\n");
        exit(0); // terminate child process
      }
    }
  }
  
  return 0;
}