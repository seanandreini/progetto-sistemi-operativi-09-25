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
#include "../include/agent.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"

#define TICKET_FILE_ADDRESS "./data/ticketList.json"
#define USERS_FILE_ADDRESS "./data/users.json"
#define AGENTS_FILE_ADDRESS "./data/agentList.json"


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

      char *fileContent = calloc(fileSize+1, sizeof(char));
      fread(fileContent, 1, fileSize, file);

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
  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

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
LoginData authenticate(char *session_token){
  FILE *file = fopen(USERS_FILE_ADDRESS, "r");
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);
  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  fclose(file);
  cJSON *user = NULL;
  LoginData userData = {0};
  cJSON_ArrayForEach(user, users){
    if(strcmp(cJSON_GetObjectItem(user, "token")->valuestring, session_token)==0){
      if(parseJSONToLoginData(user, &userData) == -1){
        printf("Error parsing json.\n");
        userData.role = GENERAL_ERROR_CODE;
      }
      return userData;
    }
  }
  userData.role = GENERAL_ERROR_CODE;
  return userData;
}

//Agent availability control function
int isAgentAvailable(char *username){
  FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
  if(file == NULL){
    printf("Error opening users file.\n");
    return 0;
  }
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);
  fclose(file);

  cJSON *ticketList = cJSON_Parse(fileContent);
  free (fileContent);

  int hasOpenTicket = 0;
  cJSON *ticket = NULL;
  cJSON_ArrayForEach(ticket, ticketList){
    cJSON *agent = cJSON_GetObjectItem(ticket, "agent");
    cJSON *agentUsername = cJSON_GetObjectItem(agent, "username");

    if(strcmp(agentUsername->valuestring, username)==0){
      cJSON *state = cJSON_GetObjectItem(ticket, "state");
      //! state->valueint == OPEN NO PERCHE' SIGNIFICA NON E' STATO ASSEGNATO
      // if(state->valueint == OPEN || state->valueint == IN_PROGRESS){
      if(state->valueint == IN_PROGRESS){
        hasOpenTicket = 1;
        break;
      }
    }
  }
  cJSON_Delete(ticketList);
  return !hasOpenTicket;
}

//Available agents loading 
cJSON* loadAvailableAgents(){
  FILE *file = fopen(USERS_FILE_ADDRESS, "r");
  if(file == NULL){
    printf("Error opening users file.\n");
    return NULL;
  }
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  fclose(file);

  cJSON *agents = cJSON_CreateArray();
  cJSON *user = NULL;
  cJSON_ArrayForEach(user, users){
    cJSON *status = cJSON_GetObjectItem(user, "status");
    cJSON *role = cJSON_GetObjectItem(user, "role");

    if(role != NULL && status != NULL && strcmp(role->valuestring, "agent")==0 && strcmp(status->valuestring, "available")==0){  
      cJSON_AddItemToArray(agents, user); 
    }
  }
  cJSON_Delete(users);

  return agents;
}

void setAgentStatus(char *username, char *status){
  FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
  if(file == NULL){
    printf("Error opening users file.\n");
    return;
  }

  int fd = fileno(file);
  flock(fd, LOCK_EX);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);

  cJSON *user = NULL;
  cJSON_ArrayForEach(user, users){
    if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, username)==0){    
      cJSON_ReplaceItemInObject(user, "status", cJSON_CreateString(status));
      break;
    }
  }
  ftruncate(fd, 0);
  fseek(file, 0, SEEK_SET);
  char *updatedContent = cJSON_Print(users);
  fwrite(updatedContent, 1, strlen(updatedContent), file); 
  free(updatedContent);

  cJSON_Delete(users);
  fclose(file);
}

void handleMessage(int clientfd, char *stringMessage){

  Message message;
  LoginData userData;
  if(!parseJSONToMessage(cJSON_Parse(stringMessage), &message)){
    printf("ERROR: invalid JSON");
    return;
  }

  // authentication
  if(message.action_code!=LOGIN_REQUEST_MESSAGE_CODE && message.action_code!=SIGNIN_MESSAGE_CODE){
    userData = authenticate(message.session_token);
    if(userData.role == GENERAL_ERROR_CODE){
      message.action_code = INFO_MESSAGE_CODE;
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
  case CREATE_TICKET_MESSAGE_CODE:{
    //* CREAZIONE TICKET
    Ticket ticket;
    if(!parseJSONToTicket(message.data, &ticket)){
      printf("ERROR: invalid json\n");
      return;
    }

    // assign ticket to an available agent (round robin)
    static int nextAgentIndex = 0;
    cJSON *availableAgents = loadAvailableAgents();
    int agentsCount = cJSON_GetArraySize(availableAgents);

    if(agentsCount > 0){
      cJSON *assignedAgent = cJSON_GetArrayItem(availableAgents, nextAgentIndex % agentsCount);
      char *agentUsername = cJSON_GetObjectItem(assignedAgent, "username")->valuestring;
      nextAgentIndex++;

      cJSON_DeleteItemFromObject(message.data, "agent");
      cJSON_AddItemToObject(message.data, "agent", assignedAgent);

      cJSON_Delete(availableAgents);
    
      int ticketId = getNextTicketId();
      cJSON_ReplaceItemInObject(message.data, "id", cJSON_CreateNumber(ticketId));

      saveTicket(message.data);
      printf("Ticket saved.\n");

      setAgentStatus(cJSON_GetObjectItem(cJSON_GetObjectItem(message.data, "agent"), "username")->valuestring, "busy");
      printf("Agent %s assigned to ticket %d. Status set to busy.\n", cJSON_GetObjectItem(cJSON_GetObjectItem(message.data, "agent"), "username")->valuestring, ticketId);
      
      char *base = "Your ticket code is: ";
      int responseLength = snprintf(NULL, 0, "%s%d", base, ticketId);
      char *response = calloc(responseLength+1, sizeof(char));
      snprintf(response, responseLength+1, "%s%d", base, ticketId);

      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString(response);
      
      char *responseString = cJSON_PrintUnformatted(parseMessageToJSON(&message));

      free(response);
      write(clientfd, responseString, strlen(responseString));
      write(clientfd, "\0", 1);
      break;
    }
    //!scegliere cosa fare se non ci sono agenti disponibili!
  }
  
  case LOGIN_REQUEST_MESSAGE_CODE:{
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
    char *fileContent = calloc(fileSize+1, sizeof(char));  

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

          message.action_code = INFO_MESSAGE_CODE;
          message.data = cJSON_CreateString("Invalid Password.\n");

          char *responseMessage = cJSON_PrintUnformatted(parseMessageToJSON(&message));

          write(clientfd, responseMessage, strlen(responseMessage));
          write(clientfd, "\0", 1);

          free(responseMessage);
          fclose(file);
          break;
        }


        //! LOGICA AGENTE STATUS -> REGISTRAZIONE/UPDATE CON CREATE/RESOLVE TICKET
        // //if user is an agent, check availability
        // cJSON *role = cJSON_GetObjectItem(user, "role");
        // if(role != NULL && strcmp(role->valuestring, "agent")==0){
        //   if(!isAgentAvailable(loginData.username)){
        //     setAgentStatus(loginData.username, "busy");
        //     printf("Agent %s logged in. Status remains busy (has open tickets).\n", loginData.username);
        //   }
        //   else{
        //     setAgentStatus(loginData.username, "available");
        //     printf("Agent %s logged in. Status set to available.\n", loginData.username);
        //   }
        // }
        
      }
    }
    fclose(file);
    
    if(!found){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Invalid username.\n");
      char *responseMessage = cJSON_PrintUnformatted(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      break;
    }

    // token generator
    char token[SESSION_TOKEN_LENGTH];
    int isUnique;
    do
    {
      srand(time(NULL)); // reset rand()
      for( int i = 0; i < SESSION_TOKEN_LENGTH; ++i){
        token[i] = '!' + rand()%93; // da 33(!) a 125 (})
      }

      isUnique = 1;
      cJSON *tempUser = NULL;
      cJSON_ArrayForEach(tempUser, users){
        if(strcmp(cJSON_GetObjectItem(tempUser, "token")->valuestring, token)==0){
          isUnique = 0;
          break;
        }
      }
    } while (!isUnique);
    

    cJSON_ReplaceItemInObject(user, "token", cJSON_CreateString(token));
    cJSON_ReplaceItemInArray(users, index, user);

    ftruncate(fd, 0);
    fseek(file, 0, SEEK_SET);
    fileContent = cJSON_Print(users);
    fwrite(fileContent, 1, strlen(fileContent), file);
    
    loginData.request_type = LOGIN_SUCCESSFUL_RESPONSE;
    strcpy(loginData.password, ""); // delete password for safety
    strncpy(loginData.token, token, SESSION_TOKEN_LENGTH+1);

    message.action_code = LOGIN_INFO_MESSAGE_CODE;
    message.data = parseLoginDataToJSON(&loginData);
    char *responseString = cJSON_PrintUnformatted(parseMessageToJSON(&message));

    write(clientfd, responseString, strlen(responseString));
    write(clientfd, "\0", 1);
    free(responseString);
    break;
  }

  case SIGNIN_MESSAGE_CODE:{
    LoginData loginData;
    if(!parseJSONToLoginData(message.data, &loginData)){
      printf("Error parsing json.\n");
      break;
    }

    FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
    flock(fileno(file), LOCK_EX);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *fileContent = calloc(fileSize+1, sizeof(char));
    fread(fileContent, 1, fileSize, file);
    
    cJSON *users = cJSON_Parse(fileContent);
    free(fileContent);
    if(users == NULL){
      printf("Error parsing users file.\n");
      cJSON_Delete(users);
      fclose(file);
      break;
    }

    cJSON *user = NULL;

    cJSON_ArrayForEach(user, users){
      if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, loginData.username)==0){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("The username is already taken.");
        char *messageString = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, messageString, strlen(messageString));
        free(messageString);
        return;
      }
    }

    //! STRUCT
    user = cJSON_CreateObject();
    cJSON_AddStringToObject(user, "role", "user");
    cJSON_AddStringToObject(user, "username", loginData.username);
    cJSON_AddStringToObject(user, "password", loginData.password);
    cJSON_AddStringToObject(user, "token", "");

    cJSON_AddItemToArray(users, user);

    ftruncate(fileno(file), 0);
    fseek(file, 0, SEEK_SET);
    char *newFileContent = cJSON_Print(users);
    fwrite(newFileContent, 1, strlen(newFileContent), file);
    fclose(file);
    free(newFileContent);

    message.action_code = INFO_MESSAGE_CODE;
    message.data = cJSON_CreateString("Utente registrato con successo.");
    char *messageString = cJSON_Print(parseMessageToJSON(&message));
    write(clientfd, messageString, strlen(messageString));
    free(messageString);
    break;
  }

  case TICKET_CONSULTATION_MESSAGE_CODE:{
    FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
    flock(fileno(file), LOCK_SH);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    cJSON *returnTicketList = cJSON_CreateArray();
    if(fileSize!=0){
      char *fileContent = calloc(fileSize+1, sizeof(char));
      fread(fileContent, 1, fileSize, file);

      cJSON *ticketList = cJSON_Parse(fileContent);
      free(fileContent);
      //! CONTROLLO FILE VALIDO
      cJSON *ticket;
      //! MAGARI ARRAY STRUCT
      cJSON_ArrayForEach(ticket, ticketList){
        if(strcmp(cJSON_GetObjectItem(ticket, "user")->valuestring, userData.username)==0){
          cJSON_AddItemToArray(returnTicketList, ticket);
        }
      }
    }


    message.action_code = TICKET_CONSULTATION_MESSAGE_CODE;
    message.data = returnTicketList;
    char *responseString = cJSON_Print(parseMessageToJSON(&message));
    write(clientfd, responseString, strlen(responseString));
    write(clientfd, "\0", 1);
    
    fclose(file);
    break;
  }

  //??ci vuole un case RESOLVE_TICKET_CODE per far cambiare lo stato del ticket e liberare l'agente??

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
  file = fopen(AGENTS_FILE_ADDRESS, "a");

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