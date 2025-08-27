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

//TODO CALLOC AL POSTO DI MALLOC

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
  long fileSize = ftell(file);
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

//Agent availability control function
/*non dovrebbe servire più
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

  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';
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
*/
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

  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  fclose(file);

  cJSON *agents = cJSON_CreateArray();
  cJSON *user = NULL;
  cJSON_ArrayForEach(user, users){
    cJSON *status = cJSON_GetObjectItem(user, "status");
    cJSON *role = cJSON_GetObjectItem(user, "role");

    if(role != NULL && status != NULL && strcmp(role->valuestring, "agent")==0 && strcmp(status->valuestring, "available")==0){  
      cJSON_AddItemToArray(agents, cJSON_Duplicate(user, 1)); //creo una copia dell'oggetto user e la aggiungo all'array agents per evitare di cancellare l'oggetto user quando cancello users
    }
  }
  cJSON_Delete(users);

  return agents;
}

void setAgentStatus(Agent *agent, int isAvailable){
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

  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);

  cJSON *user = NULL;
  cJSON_ArrayForEach(user, users){
    if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, agent->username)==0){    
      cJSON_ReplaceItemInObject(user, "isAvailable", cJSON_CreateNumber(isAvailable));
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

// date comparison function
int isEarlierDate(Date *date1, Date *date2){
  if(date1->anno < date2->anno) return 1;
  if(date1->anno > date2->anno) return 0;
  if(date1->mese < date2->mese) return 1;
  if(date1->mese > date2->mese) return 0;
  if(date1->giorno < date2->giorno) return 1;
  return 0;
}

int findBestOpenTicket(Ticket *bestTicket){
  int found = 0;
  int highestPriority = MIN_PRIORITY - 1; // lower than the lowest priority
  Date earliestDate = {31, 12, 9999}; // far future date

  FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
  if(file == NULL){
    printf("Error opening ticket file.\n");
    return 0;
  }
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if(fileSize == 0){
    fclose(file);
    return 0; // no tickets
  }

  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';
  fclose(file);

  cJSON *ticketList = cJSON_Parse(fileContent);
  free (fileContent);

  cJSON *bestTicketJSON = NULL;
  cJSON_ArrayForEach(bestTicketJSON, ticketList){
    if(cJSON_GetObjectItem(bestTicketJSON, "state")->valueint == OPEN){
      cJSON *currentPriority = cJSON_GetObjectItem(bestTicketJSON, "priority");
      cJSON *currentDateJSON = cJSON_GetObjectItem(bestTicketJSON, "date");
      Date currentDate;
      if(!parseJSONToDate(currentDateJSON, &currentDate)) continue; // error parsing date, skip this ticket
      if((currentPriority->valueint > highestPriority) || (currentPriority->valueint == highestPriority && isEarlierDate(&currentDate, &earliestDate))){
        if(parseJSONToTicket(bestTicketJSON, bestTicket)){
          highestPriority = currentPriority->valueint;
          earliestDate = currentDate;
          found = 1;
        }
        //??se il ticket con errore nel parse era il migliore??
      }
    }
  }
  cJSON_Delete(ticketList);
  return found;
}
 
int updateTicket(int ticketId, cJSON *updatesJSON){ //updatesJSON is an object with the fields to update
  //??solo l'admin può cambiare l'agente, e solo l'agente assegnato al ticket può cambiarne lo stato??
  int updated = 0;

  FILE *file = fopen(TICKET_FILE_ADDRESS, "r+");
  if(file == NULL){
    printf("Error opening ticket file.\n");
    return 0;
  }
  int fd = fileno(file);
  flock(fd, LOCK_EX);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if(fileSize == 0){
    fclose(file);
    return 0; // no tickets
  }

  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';

  cJSON *ticketList = cJSON_Parse(fileContent);
  free (fileContent);

  cJSON *ticket = NULL;
  cJSON_ArrayForEach(ticket, ticketList){
    if(cJSON_GetObjectItem(ticket, "id")->valueint == ticketId){
      cJSON *update = NULL;
      cJSON_ArrayForEach(update, updatesJSON){
        cJSON_ReplaceItemInObject(ticket, update->string, cJSON_Duplicate(update, 1));
      }
      updated = 1;
      break;
    }
  }

  if(updated){
    ftruncate(fd, 0);
    fseek(file, 0, SEEK_SET);
    char *updatedContent = cJSON_Print(ticketList);
    fwrite(updatedContent, 1, strlen(updatedContent), file); 
    free(updatedContent);
  }

  cJSON_Delete(ticketList);
  fclose(file);
  
  return updated;
  
}

//assign an available agent to a ticket already created 
void assignTicketToAgent(int ticketId, Agent *agent){
  cJSON *agentJSON = parseAgentToJSON(agent);
  cJSON *updates = cJSON_CreateObject();
  cJSON_AddItemToObject(updates, "agent", agentJSON);
  cJSON_AddNumberToObject(updates, "state", IN_PROGRESS);
  if(updateTicket(ticketId, updates)){
    setAgentStatus(agent, 0); // set agent to busy
    printf("Ticket %d assigned to agent %s and state set to IN_PROGRESS.\n", ticketId, agent->username);
  }
  else{
    printf("Error: Ticket %d not found.\n", ticketId);
  }
  cJSON_Delete(updates);
}

int getTicketById(int ticketId, Ticket *ticket){
  int found = 0;

  FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
  if(file == NULL){
    printf("Error opening ticket file.\n");
    return 0;
  }
  flock(fileno(file), LOCK_SH);

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  if(fileSize == 0){
    fclose(file);
    return 0; // no tickets
  }

  char *fileContent = malloc(fileSize + 1);
  fread(fileContent, 1, fileSize, file);
  fileContent[fileSize] = '\0';
  fclose(file);

  cJSON *ticketList = cJSON_Parse(fileContent);
  free (fileContent);

  cJSON *ticketJSON = NULL;
  cJSON_ArrayForEach(ticketJSON, ticketList){
    if(cJSON_GetObjectItem(ticketJSON, "id")->valueint == ticketId){
      if(parseJSONToTicket(ticketJSON, ticket)){
        found = 1;
        break;
      }
    }
  }
  cJSON_Delete(ticketList);
  return found;
}

void handleMessage(int clientfd, char *stringMessage){

  Message message;
  char *role;
  if(!parseJSONToMessage(cJSON_Parse(stringMessage), &message)){
    printf("ERROR: invalid JSON");
    return;
  }

  // authentication
  if(message.action_code!=LOGIN_REQUEST_MESSAGE_CODE && message.action_code!=SIGNIN_MESSAGE_CODE){
    role = authenticate(message.session_token);
    if(role==NULL){
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

    cJSON_Delete(message.data); 
    
    int agentWasAssigned = 0;
    Agent assignedAgent;

    cJSON *availableAgents = loadAvailableAgents();
    int agentsCount = cJSON_GetArraySize(availableAgents);
    
    if(agentsCount > 0){
      printf("Available agents found: %d\n", agentsCount);
      static int nextAgentIndex = 0; // static to keep its value between function calls
      cJSON *agentJSON = cJSON_GetArrayItem(availableAgents, nextAgentIndex % agentsCount);
      nextAgentIndex++;

      if(parseJSONToAgent(agentJSON, &assignedAgent)){
        agentWasAssigned = 1;
        printf("Agent %s assigned to ticket.\n", assignedAgent.username);
        ticket.state = IN_PROGRESS;
        ticket.agent = assignedAgent;
      }
    }

    if(!agentWasAssigned){
      printf("No available agents, ticket will be created with OPEN status.\n");
      ticket.state = OPEN;
      ticket.agent.code = -1;
      strcpy(ticket.agent.username, "N/A"); // no agent assigned
    }
    cJSON_Delete(availableAgents);
    ticket.id = getNextTicketId();
    cJSON *jsonTicket = parseTicketToJSON(&ticket);
    saveTicket(jsonTicket);
    cJSON_Delete(jsonTicket);

    if(agentWasAssigned){
      setAgentStatus(assignedAgent.username, 0);
      printf("Agent %s status set to busy.\n", assignedAgent.username);
    }

    // ??send response to client??
    break;
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

  case RESOLVE_TICKET_CODE:{
    if(strcmp(role, "agent")!=0){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Only agents can resolve tickets.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    int ticketId = cJSON_GetObjectItem(message.data, "ticket_id")->valueint;
    Ticket ticketToClose;

    if(!getTicketById(ticketId, &ticketToClose)){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Ticket not found.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    if(strcmp(userData.username, ticketToClose.agent.username)!=0){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("You can only resolve tickets assigned to you.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    cJSON *stateUpdate = cJSON_CreateObject();
    cJSON_AddNumberToObject(stateUpdate, "state", CLOSED);
    updateTicket(ticketId, stateUpdate);
    cJSON_Delete(stateUpdate);
    printf("Ticket %d resolved by agent %s.\n", ticketId, userData.username);

    Ticket nextTicket;
    if(findBestOpenTicket(&nextTicket)){
      assignTicketToAgent(nextTicket.id, &userData);
    }
    else{
      setAgentStatus(&userData, 1); // set agent to available
      printf("No open tickets available. Agent %s set to available.\n", userData.username);
    }
    
    cJSON_Delete(message.data);
    message.action_code = INFO_MESSAGE_CODE;
    message.data = cJSON_CreateString("Ticket resolved successfully.");
    char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
    write(clientfd, responseMessage, strlen(responseMessage));
    write(clientfd, "\0", 1);
    free(responseMessage);
    
    break;
  }

  case UPDATE_TICKET_CODE:{
    if(strcmp(role, "admin")!=0){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Only admins can update tickets.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    int ticketId = cJSON_GetObjectItem(message.data, "ticket_id")->valueint;
    cJSON *updates = cJSON_GetObjectItem(message.data, "updates");

    if(updates == NULL || !cJSON_IsObject(updates)){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Invalid updates format.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    if(updateTicket(ticketId, updates)){
      cJSON_Delete(message.data);
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Ticket updated successfully.");
    }
    else{
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Ticket not found.");
    }

    char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
    write(clientfd, responseMessage, strlen(responseMessage));
    write(clientfd, "\0", 1);
    free(responseMessage);
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