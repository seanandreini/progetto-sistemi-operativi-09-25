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
#include <time.h>
#include "../lib/cJSON/cJSON.h"
#include "../include/jsonParser.h"
#include "../include/functions.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"

#define TICKET_FILE_ADDRESS "./data/ticketList.json"
#define USERS_FILE_ADDRESS "./data/users.json"


//! RICORDA DI VEDERE DIMENSIONE ARRAY LETTURA



void saveTicket(cJSON *ticket){
  FILE *file = fopen(TICKET_FILE_ADDRESS, "r+");
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
  FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
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

// return role if token is valid, NULL otherwise
char *authenticate(char *session_token){
  FILE *file = fopen(USERS_FILE_ADDRESS, "r");
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  int fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *fileContent = malloc(fileSize);
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  fclose(file);
  cJSON *user = NULL;
  cJSON_ArrayForEach(user, users){
    if(strcmp(cJSON_GetObjectItem(user, "token")->valuestring, session_token)==0){
      return cJSON_GetObjectItem(user, "role")->valuestring;
    }
  }

  return NULL;
}

void handleMessage(int clientfd, char *stringMessage){

  Message message;
  char *role;
  if(!parseJSONToMessage(cJSON_Parse(stringMessage), &message)){
    printf("ERROR: invalid JSON");
    return;
  }

  if(message.action_code!=LOGIN_REQUEST_CODE){
    role = authenticate(message.session_token);
    if(role==NULL){
      message.action_code = MESSAGE_CODE;
      message.data = cJSON_CreateString("Invalid session, try logging in.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      return;
    }
  }

  switch (message.action_code)
  {
  case CREATE_TICKET_CODE:{
    //* CREAZIONE TICKET
    Ticket ticket;
    if(!parseJSONToTicket(message.data, &ticket)){
      printf("ERROR: invalid json\n");
      return;
    }
    int ticketId = getNextTicketId();
    cJSON_ReplaceItemInObject(message.data, "id", cJSON_CreateNumber(ticketId));

    saveTicket(message.data);
    printf("Ticket saved.\n");

    char *base = "Your ticket code is: ";
    int responseLength = snprintf(NULL, 0, "%s%d", base, ticketId);
    char *response = malloc(responseLength+1);
    snprintf(response, responseLength+1, "%s%d", base, ticketId);

    message.action_code = MESSAGE_CODE;
    message.data = cJSON_CreateString(response);
    
    char *responseString = cJSON_PrintUnformatted(parseMessageToJSON(&message));

    free(response);
    write(clientfd, responseString, strlen(responseString));
    write(clientfd, "\0", 1);
    break;
  }
  
  case LOGIN_REQUEST_CODE:{
    LoginData loginData;
    
    if(!parseJSONToLoginData(message.data, &loginData)){
      printf("ERROR: invalid json.\n");
      break;
    }

    FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
    int fd = fileno(file);
    flock(fd, LOCK_SH);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *fileContent = malloc(fileSize+1);  

    short found = 0;
    cJSON *users = NULL;
    cJSON *user = NULL;
    int index=-1;

    if(fileSize!=0){
      fread(fileContent, 1, fileSize, file);
      users = cJSON_Parse(fileContent);
      free(fileContent);
      
      cJSON_ArrayForEach(user, users){
        if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, loginData.username)==0){
          found = 1;
          index++;
          break;
        }
      }
      if(found){
        if(strcasecmp(cJSON_GetObjectItem(user, "password")->valuestring, loginData.password)!=0){

          message.action_code = MESSAGE_CODE;
          message.data = cJSON_CreateString("Invalid Password.\n");

          char *responseMessage = cJSON_PrintUnformatted(parseMessageToJSON(&message));

          write(clientfd, responseMessage, strlen(responseMessage));
          write(clientfd, "\0", 1);

          free(responseMessage);
          fclose(file);
          break;
        }
      }
    }
    fclose(file);
    
    if(!found){
      message.action_code = MESSAGE_CODE;
      message.data = cJSON_CreateString("Invalid username.\n");
      char *responseMessage = cJSON_PrintUnformatted(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      break;
    }

    // token generator
    srand(time(NULL)); // reset rand()
    char token[SESSION_TOKEN_LENGTH];
    for( int i = 0; i < SESSION_TOKEN_LENGTH; ++i){
      token[i] = '!' + rand()%93; // da 33(!) a 125 (})
    }

    cJSON_ReplaceItemInObject(user, "token", cJSON_CreateString(token));
    cJSON_ReplaceItemInArray(users, index, user);

    ftruncate(fd, 0);
    fseek(file, 0, SEEK_SET);
    fileContent = cJSON_Print(users);
    fwrite(fileContent, 1, strlen(fileContent), file);
    
    loginData.request_type = LOGIN_SUCCESSFUL_RESPONSE;
    strcpy(loginData.password, ""); // delete password for safety
    strncpy(loginData.token, token, SESSION_TOKEN_LENGTH+1);

    message.action_code = LOGIN_INFO_CODE;
    message.data = parseLoginDataToJSON(&loginData);
    char *responseString = cJSON_PrintUnformatted(parseMessageToJSON(&message));

    write(clientfd, responseString, strlen(responseString));
    write(clientfd, "\0", 1);
    free(responseString);
    break;
  }



  default:
    printf("ERROR: Invalid action_code.\n");
    break;
  }

}

int main(int argc, char *argv[]){
  
  int socketfd, clientfd;
  socklen_t clientAddressSize;
  struct sockaddr_in serverAddress, clientAddress;

  // creates files if it doesn't exist
  FILE *file = fopen(TICKET_FILE_ADDRESS, "a");
  fclose(file);
  file = fopen(USERS_FILE_ADDRESS, "a");
  fclose(file);

  signal(SIGCHLD, SIG_IGN); // anti zombie handler

  // creazione socket
  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error creating socket");
    exit(-1);
  }
  printf("Server socket created successfully.\n");

  //! PER FAR RIMANDARE IL SERVER
  int reuse=1;
  if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse))<0){
    printf("Errore SO_REUSEADDR");
  }

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
        handleMessage(clientfd, message);
        close(clientfd);
        printf("Connection closed.\n");
        exit(0); // terminate child process
      }
    }
  }
  

  return 0;
}