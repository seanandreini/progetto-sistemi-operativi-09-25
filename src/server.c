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

//! RICORDA DI CAMBIARE IN -1 ERRORE PARSEJSON


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
User authenticate(char *session_token){
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
  User userData = {0};
  cJSON_ArrayForEach(user, users){
    if(strcmp(cJSON_GetObjectItem(user, "token")->valuestring, session_token)==0){
      if(parseJSONToUser(user, &userData) == -1){
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
      //! state->valueint == OPEN_STATE_STATE NO PERCHE' SIGNIFICA NON E' STATO ASSEGNATO
      // if(state->valueint == OPEN_STATE_STATE || state->valueint == IN_PROGRESS_STATE){
      if(state->valueint == IN_PROGRESS_STATE){
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

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  fclose(file);

  cJSON *agents = cJSON_CreateArray();
  cJSON *user = NULL;
  cJSON_ArrayForEach(user, users){
    cJSON *isAvailable = cJSON_GetObjectItem(user, "isAvailable");
    cJSON *role = cJSON_GetObjectItem(user, "role");

    if(role != NULL && isAvailable != NULL && 
      strcmp(role->valuestring, "agent")==0 && isAvailable->valueint){  
      cJSON_AddItemToArray(agents, user); 
    }
  }
  //! cJSON_Delete(users);

  return agents;
}

void setAgentStatus(char *username, int isAvailable){
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
  if(date1->year < date2->year) return 1;
  if(date1->year > date2->year) return 0;
  if(date1->month < date2->month) return 1;
  if(date1->month > date2->month) return 0;
  if(date1->day < date2->day) return 1;
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
    if(cJSON_GetObjectItem(bestTicketJSON, "state")->valueint == OPEN_STATE){
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
void assignTicketToAgent(int ticketId, User *agent){
  // cJSON *agentJSON = parseUserToJSON(agent);
  cJSON *updates = cJSON_CreateObject();
  cJSON_AddStringToObject(updates, "agent", agent->username);
  cJSON_AddNumberToObject(updates, "state", IN_PROGRESS_STATE);
  if(updateTicket(ticketId, updates)){
    setAgentStatus(agent->username, 0); // set agent to busy
    printf("Ticket %d assigned to agent %s and state set to IN_PROGRESS_STATE.\n", ticketId, agent->username);
  }
  else{
    printf("Error: Ticket %d not found.\n", ticketId);
  }
  //!cJSON_Delete(updates);
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

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);
  fclose(file);

  cJSON *ticketList = cJSON_Parse(fileContent);
  free(fileContent);

  printf("prova");

  cJSON *ticketJSON = NULL;
  cJSON_ArrayForEach(ticketJSON, ticketList){
    printf("ticket: %d\n", ticketId);
    if(cJSON_GetObjectItem(ticketJSON, "id")->valueint == ticketId){
      found = 1;
      break;
    }
  }

  printf("found%d\n", found);
  //!cJSON_Delete(ticketList);
  return found;
}

void handleMessage(int clientfd, char *stringMessage){
  Message message = {0};
  User userData = {0};

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

    strcpy(ticket.user, userData.username);
    
    int agentWasAssigned = 0;
    User assignedAgent;

    cJSON *availableAgents = loadAvailableAgents();
    int agentsCount = cJSON_GetArraySize(availableAgents);
    
    if(agentsCount > 0){
      printf("Available agents found: %d\n", agentsCount);
      static int nextAgentIndex = 0; // static to keep its value between function calls
      cJSON *agentJSON = cJSON_GetArrayItem(availableAgents, nextAgentIndex % agentsCount);
      nextAgentIndex++;


      if(!parseJSONToUser(agentJSON, &assignedAgent)){
        printf("Error parsing agent json.\n");
        return;
      }

      agentWasAssigned = 1;
      printf("Agent %s assigned to ticket.\n", assignedAgent.username);
      ticket.state = IN_PROGRESS_STATE;
      strcpy(ticket.agent, assignedAgent.username);
    }

    if(!agentWasAssigned){
      printf("No available agents, ticket will be created with OPEN_STATE status.\n");
      ticket.state = OPEN_STATE;
      strcpy(ticket.agent, "N/A"); // no agent assigned
    }
    //!cJSON_Delete(availableAgents);

    ticket.id = getNextTicketId();
    cJSON *jsonTicket = parseTicketToJSON(&ticket);
    saveTicket(jsonTicket);
    //! cJSON_Delete(jsonTicket);


    //! RICORDA DI DECIDERE LOGICA RIDONDANZA ISAVAILABLE
    if(agentWasAssigned){
      setAgentStatus(assignedAgent.username, 0);
      printf("Agent %s status set to busy.\n", assignedAgent.username);
    }

    message.action_code = INFO_MESSAGE_CODE;
    char *base = "Ticket created successfully, your ticket code is ";
    int fullStringLength = snprintf(NULL, 0, "%s%d", base, ticket.id);
    char *fullString = malloc(fullStringLength+1);
    snprintf(fullString, fullStringLength+1, "%s%d.", base, ticket.id);
    message.data = cJSON_CreateString(fullString);
    
    char *response = cJSON_Print(parseMessageToJSON(&message));
    write(clientfd, response, strlen(response));
    write(clientfd, "\0", 1);

    break;
  }
  
  case LOGIN_REQUEST_MESSAGE_CODE:{    
    User userData = {0};

    if(!parseJSONToUser(message.data, &userData)){
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
        if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, userData.username)==0){
          found = 1;
          index++;
          break;
        }
      }
      if(found){
        if(strcasecmp(cJSON_GetObjectItem(user, "password")->valuestring, userData.password)!=0){

          message.action_code = INFO_MESSAGE_CODE;
          message.data = cJSON_CreateString("Invalid Password.");

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
    
    if(!found){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Invalid username.");
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
    fclose(file);

    strcpy(userData.password, ""); // delete password for safety
    strcpy(message.session_token, token);

    message.action_code = LOGIN_INFO_MESSAGE_CODE;
    message.data = parseUserToJSON(&userData);
    char *responseString = cJSON_PrintUnformatted(parseMessageToJSON(&message));

    write(clientfd, responseString, strlen(responseString));
    write(clientfd, "\0", 1);
    free(responseString);
    break;
  }

  case SIGNIN_MESSAGE_CODE:{
    User userData = {0};
    if(!parseJSONToUser(message.data, &userData)){
      printf("Error parsing user from message data.\n");
      break;
    }

    FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
    flock(fileno(file), LOCK_EX);

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    cJSON *fileUsers = cJSON_CreateArray();

    if(fileSize!=0){
      char *fileContent = calloc(fileSize+1, sizeof(char));
      fread(fileContent, 1, fileSize, file);
      fileUsers = cJSON_Parse(fileContent);
      free(fileContent);

      if(fileUsers == NULL){
        printf("Error parsing users file.\n");
        cJSON_Delete(fileUsers);
        fclose(file);
        break;
      }
  
      cJSON *fileUser = NULL;
  
      cJSON_ArrayForEach(fileUser, fileUsers){
        if(strcmp(cJSON_GetObjectItem(fileUser, "username")->valuestring, userData.username)==0){
          message.action_code = INFO_MESSAGE_CODE;
          message.data = cJSON_CreateString("The username is already taken.");
          char *messageString = cJSON_Print(parseMessageToJSON(&message));
          write(clientfd, messageString, strlen(messageString));
          free(messageString);
          return;
        }
      }
    }

    //! STRUCT
    
    // user = cJSON_CreateObject();
    // cJSON_AddStringToObject(user, "role", "user");
    // cJSON_AddStringToObject(user, "username", loginData.username);
    // cJSON_AddStringToObject(user, "password", loginData.password);
    // cJSON_AddStringToObject(user, "token", "");
    userData.role = USER_ROLE;
    userData.isAvailable = -1;

    cJSON_AddItemToArray(fileUsers, parseUserToJSON(&userData));

    ftruncate(fileno(file), 0);
    fseek(file, 0, SEEK_SET);
    char *newFileContent = cJSON_Print(fileUsers);
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

  case RESOLVE_TICKET_MESSAGE_CODE:{
    if(userData.role!=AGENT_ROLE){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Only agents can resolve tickets.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    Ticket ticket = {0};
    //! RIMASTO A PARSARE STO ROBO, AGGIUNGI CONTROLLO ERRORE, NON ANCORA TESTATO
    parseJSONToTicket(message.data, &ticket);
    

    if(!getTicketById(ticket.id, &ticket)){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Ticket not found.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    if(strcmp(userData.username, ticket.agent)!=0){
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("You can only resolve tickets assigned to you.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }

    cJSON *stateUpdate = cJSON_CreateObject();
    cJSON_AddNumberToObject(stateUpdate, "state", CLOSED_STATE);
    updateTicket(ticket.id, stateUpdate);
    //!cJSON_Delete(stateUpdate);
    printf("Ticket %d resolved by agent %s.\n", ticket.id, userData.username);

    Ticket nextTicket;
    if(findBestOpenTicket(&nextTicket)){
      assignTicketToAgent(nextTicket.id, &userData);
    }
    else{
      setAgentStatus(userData.username, 1); // set agent to available
      printf("No open tickets available. Agent %s set to available.\n", userData.username);
    }
    
    //!cJSON_Delete(message.data);
    message.action_code = INFO_MESSAGE_CODE;
    message.data = cJSON_CreateString("Ticket resolved successfully.");
    char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
    write(clientfd, responseMessage, strlen(responseMessage));
    write(clientfd, "\0", 1);
    free(responseMessage);
    
    break;
  }

  case UPDATE_TICKET_MESSAGE_CODE:{
    if(userData.role != ADMIN_ROLE){
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
        char receivedMessage[256]= {0};
        readMessage(clientfd, receivedMessage);
        // printf("Received from client n.%d: %s", clientfd, cJSON_Print(cJSON_Parse(receivedMessage)));
        handleMessage(clientfd, receivedMessage);
        close(clientfd);
        printf("Connection closed.\n");
        exit(0); // terminate child process
      }
    }
  }
  

  return 0;
}