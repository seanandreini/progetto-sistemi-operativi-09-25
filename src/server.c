#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/file.h>
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



void saveTicket(cJSON *ticket){
  FILE *file = fopen("./data/ticketList.json", "r+");
  int fd = fileno(file);
  flock(fd, LOCK_EX);

  if(file != NULL){
    // printf("File exists.\n");

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    cJSON *jsonTicketList;
    if(fileSize != 0){
      // printf("File is not empty.\n");

      char *fileContent = malloc(fileSize + 1);
      fread(fileContent, 1, fileSize, file);
      fileContent[fileSize] = '\0';

      jsonTicketList = cJSON_Parse(fileContent);
      free(fileContent);
    }
    else{
      // printf("File is empty.\n");

      jsonTicketList = cJSON_CreateArray();
    }

    cJSON_AddItemToArray(jsonTicketList, ticket);
    char *newContent = cJSON_Print(jsonTicketList);

    ftruncate(fd, 0);
    fseek(file, 0, SEEK_SET);
    fwrite(newContent, 1, strlen(newContent), file);

    free(newContent);
    cJSON_Delete(jsonTicketList);

    fclose(file);
  }
}

int getNextTicketId(){
  int maxId = 0;
  FILE *file = fopen("./data/ticketList.json", "r");
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';

  cJSON *jsonTicketList = cJSON_Parse(fileContent);
  cJSON *ticket = cJSON_CreateArray();

  cJSON_ArrayForEach(ticket, jsonTicketList){
    int currentId = cJSON_GetObjectItem(ticket, "id")->valueint;
    if(currentId > maxId) maxId = currentId;
  }

  free(fileContent);
  cJSON_Delete(ticket);
  cJSON_Delete(jsonTicketList);

  fclose(file);

  return maxId+1;
}

cJSON *createJsonMessage(int code, char *data){
  cJSON *jsonMessage = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonMessage, "ACTION_CODE", code);
  cJSON_AddStringToObject(jsonMessage, "data", data);
  return jsonMessage;
}

char *handleMessage(char *message){
  printf("%s", message);
  cJSON *jsonMessage = cJSON_Parse(message);
  int actionCode = cJSON_GetObjectItem(jsonMessage, "ACTION_CODE")->valueint;
  cJSON *jsonData = cJSON_GetObjectItem(jsonMessage, "data");

  switch (actionCode)
  {
  case CREATE_TICKET_CODE:{
    //* CREAZIONE TICKET
    int ticketId = getNextTicketId();
    cJSON_ReplaceItemInObject(jsonData, "id", cJSON_CreateNumber(ticketId));

    saveTicket(jsonData);
    printf("Ticket saved.\n");

    char *base = "Your ticket code is: ";
    int responseLength = snprintf(NULL, 0, "%s%d", base, ticketId);
    char *response = malloc(responseLength+1);
    snprintf(response, responseLength+1, "%s%d", base, ticketId);
    
    jsonMessage = createJsonMessage(MESSAGE_CODE, response);
    char *responseString = cJSON_PrintUnformatted(jsonMessage);

    free(response);
    return responseString;
  }
  
  case LOGIN_REQUEST:{

    

    char *username = cJSON_GetObjectItem(jsonData, "username")->valuestring;
    char *password = cJSON_GetObjectItem(jsonData, "password")->valuestring;
    printf("Login request with username: %s and password: %s.\n", username, password);

    FILE *file = fopen("./data/users.json", "r+");
    flock(fileno(file), LOCK_SH);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *fileContent = malloc(fileSize+1);
    fread(fileContent, 1, fileSize, file);


    cJSON *users = cJSON_Parse(fileContent);
    free(fileContent);
    cJSON *user = NULL;
    short found = 0;
    cJSON_ArrayForEach(user, users){
      if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, username)==0){
        found = 1;
        break;
      }
    }
    if(found){
      if(strcasecmp(cJSON_GetObjectItem(user, "password")->valuestring, password)!=0){
        free(username);
        free(password);
        return cJSON_PrintUnformatted(createJsonMessage(MESSAGE_CODE, "Invalid Password."));
      }
    }
    else{
      free(username);
      free(password);
      return cJSON_PrintUnformatted(createJsonMessage(MESSAGE_CODE, "Invalid username."));
    }

    // password giusta
    return cJSON_PrintUnformatted(createJsonMessage(MESSAGE_CODE, "Login successfull."));
    


    free(username);
    free(password);

    break;
  }

  default:
    printf("ERROR: Invalid action_code.\n");
    return "";
  }
}

int main(int argc, char *argv[]){
  
  int socketfd, clientfd;
  socklen_t clientAddressSize;
  struct sockaddr_in serverAddress, clientAddress;

  // creates files if it doesn't exist
  FILE *file = fopen("./data/ticketList.json", "a");
  fclose(file);
  file = fopen("./data/users.json", "a");
  fclose(file);

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




      if(fork() != 0) {
        // parent process
        close(clientfd); // close the client socket
      }
      else {
        // child process
        close(socketfd); // close the listening socket

        printf("Connection accepted from %s:%d\n", inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
        
        //lettura messaggio dal client e stampa
        char message[256]= {0};
        readMessage(clientfd, message);
        // printf("Received from client n.%d: %s", clientfd, message);
        
        char *responseString = handleMessage(message);
        write(clientfd, responseString, strlen(responseString));
        write(clientfd, "\0", 1);
        close(clientfd);
        printf("Connection closed.\n");
        exit(0); // terminate child process
      }
    }
  }
  

  return 0;
}