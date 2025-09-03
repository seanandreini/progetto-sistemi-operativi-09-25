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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../lib/cJSON/cJSON.h"
#include "../include/jsonParser.h"
#include "../include/functions.h"
#include "../include/agent.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"

#define TICKET_FILE_ADDRESS "./data/ticketList.json"
#define USERS_FILE_ADDRESS "./data/users.json"
#define DATA_FOLDER_ADDRESS "./data"

//! SI POTREBBE RESETTARE IL TOKEN AL LOGOUT
//! PRINTUNFORMATTED

// takes ticket in input and adds it to ticketList.json
int saveTicket(cJSON *ticket){
  FILE *file = fopen(TICKET_FILE_ADDRESS, "r+");
  if(file == NULL){
    printf("Error opening ticketList file.\n");
    return 0;
  }

  int fd = fileno(file);
  flock(fd, LOCK_EX);
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  cJSON *jsonTicketList = NULL;

  // if file is not empty creates cJSON_Array parsing fileContent
  if(fileSize != 0){
    char *fileContent = calloc(fileSize+1, sizeof(char));
    fread(fileContent, 1, fileSize, file);

    jsonTicketList = cJSON_Parse(fileContent);
    free(fileContent);
    if(!cJSON_IsArray(jsonTicketList)) return 0;
  }
  else{
    jsonTicketList = cJSON_CreateArray();
  }

  // adds new ticket to ticket list read from file
  cJSON_AddItemToArray(jsonTicketList, ticket);
  char *newContent = cJSON_Print(jsonTicketList);

  // empties file and writes new updated list
  ftruncate(fd, 0);
  fseek(file, 0, SEEK_SET);
  fwrite(newContent, 1, strlen(newContent), file);

  free(newContent);
  cJSON_Delete(jsonTicketList);
  fclose(file);


  return 1;
}

// return first available ticket id
int getNextTicketId(){
  FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
  if(file == NULL){
    printf("Error opening ticketList file.\n");
    return 0;
  }

  int maxId = 0;
  flock(fileno(file), LOCK_SH);
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);

  if(fileSize != 0){
    fseek(file, 0, SEEK_SET);
    char *fileContent = calloc(fileSize+1, sizeof(char));
    fread(fileContent, 1, fileSize, file);
  
    // parses content to cJSON_Array
    cJSON *jsonTicketList = cJSON_Parse(fileContent);
    cJSON *ticket = NULL;
  
    cJSON_ArrayForEach(ticket, jsonTicketList){
      int currentId = cJSON_GetObjectItem(ticket, "id")->valueint;
      if(currentId > maxId) maxId = currentId;
    }
  
    free(fileContent);
    cJSON_Delete(jsonTicketList);
  }
  fclose(file);
  return maxId+1;
}

// returns userData with user info, empty userData with role set as GENERAL_ERROR_CODE otherwise
User authenticate(char *session_token){
  User userData = {0};

  // if this doesn't get overridden authentication has failed
  userData.role = GENERAL_ERROR_CODE;

  // can't authenticate if token is invalid
  if(strlen(session_token)==0) return userData;

  FILE *file = fopen(USERS_FILE_ADDRESS, "r");
  flock(fileno(file), LOCK_SH);
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);

  // if file is empty there's no user to authenticate
  if(fileSize == 0) return userData;

  fseek(file, 0, SEEK_SET);
  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);
  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  fclose(file);
  
  if(!cJSON_IsArray(users)) return userData;

  cJSON *user = NULL;
  // goes through users.json
  cJSON_ArrayForEach(user, users){
    // if tokens are the same copies user info into userData
    if(strcmp(cJSON_GetObjectItem(user, "token")->valuestring, session_token)==0){
      if(parseJSONToUser(user, &userData) == GENERAL_ERROR_CODE){
        printf("Error parsing json.\n");
      }
      return userData;
    }
  }
  return userData;
}

// available agents loading returns cJSON array if succeds, NULL otherwise
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

  // returns new empty array if there are no users
  if(fileSize == 0){
    fclose(file);
    return cJSON_CreateArray();
  }

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);

  if(!cJSON_IsArray(users)){
    printf("Error parsing users file.\n");
    return NULL;
  }

  free(fileContent);
  fclose(file);

  cJSON *agents = cJSON_CreateArray();
  cJSON *user = NULL;

  // goes through user list, if it's an agent and it's available it gets added to the list
  cJSON_ArrayForEach(user, users){
    cJSON *isAvailable = cJSON_GetObjectItem(user, "isAvailable");
    cJSON *role = cJSON_GetObjectItem(user, "role");
    if(role != NULL && isAvailable != NULL && 
      role->valueint==AGENT_ROLE && isAvailable->valueint){  
      cJSON_AddItemToArray(agents, cJSON_Duplicate(user, 1)); 
    }
  }
  //! cJSON_Delete(users);

  return agents;
}

// changes agent availability in users.json
int setAgentStatus(char *username, int isAvailable){
  FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
  if(file == NULL){
    printf("Error opening users file.\n");
    return 0;
  }

  int fd = fileno(file);
  flock(fd, LOCK_EX);
  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  if(fileSize == 0) return 0;
  fseek(file, 0, SEEK_SET);

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *users = cJSON_Parse(fileContent);
  free(fileContent);
  if(!cJSON_IsArray(users)){
    printf("Error parsing users file.\n");
    return 0;
  }

  cJSON *user = NULL;
  // goes through agent list and changes agent status if it finds it
  cJSON_ArrayForEach(user, users){
    if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, username)==0){    
      cJSON_ReplaceItemInObject(user, "isAvailable", cJSON_CreateNumber(isAvailable));
      break;
    }
  }

  // overrites file with updated list
  ftruncate(fd, 0);
  fseek(file, 0, SEEK_SET);
  char *updatedContent = cJSON_Print(users);
  fwrite(updatedContent, 1, strlen(updatedContent), file); 
  free(updatedContent);
  cJSON_Delete(users);
  fclose(file);

  return 1;
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

// finds ticket with highest priority, if they're the same older date
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

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);
  fclose(file);

  cJSON *ticketList = cJSON_Parse(fileContent);
  free (fileContent);
  if(!cJSON_IsArray(ticketList)){
    printf("Error parsing ticketList.\n");
    return 0;
  }

  // goes through ticketList
  cJSON *bestTicketJSON = NULL;
  cJSON_ArrayForEach(bestTicketJSON, ticketList){
    if(cJSON_GetObjectItem(bestTicketJSON, "state")->valueint == OPEN_STATE){
      cJSON *currentPriority = cJSON_GetObjectItem(bestTicketJSON, "priority");
      cJSON *currentDateJSON = cJSON_GetObjectItem(bestTicketJSON, "date");
      Date currentDate;
      if(!parseJSONToDate(currentDateJSON, &currentDate)){
        printf("Error parsing ticket json.\n");
        return 0;
      }

      // sets highest priority and earliest date
      if((currentPriority->valueint > highestPriority) || 
        (currentPriority->valueint == highestPriority && 
          isEarlierDate(&currentDate, &earliestDate))){
        if(!parseJSONToTicket(bestTicketJSON, bestTicket)){
          printf("Error parsing ticket json.\n");
          return 0;
        }
        highestPriority = currentPriority->valueint;
        earliestDate = currentDate;
        found = 1;
      }
    }
  }
  cJSON_Delete(ticketList);
  return found;
}
 
// updates ticket with info passed as json
int updateTicket(int ticketId, cJSON *updatesJSON){ 
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

  char *fileContent = calloc(fileSize+1, sizeof(char));
  fread(fileContent, 1, fileSize, file);

  cJSON *ticketList = cJSON_Parse(fileContent);
  free (fileContent);

  if(!cJSON_IsArray(ticketList)){
    printf("Error parsing ticket json.\n");
    return 0;
  }

  cJSON *ticket = NULL;
  // goes through ticket list and updates info when it finds the right one
  cJSON_ArrayForEach(ticket, ticketList){
    if(cJSON_GetObjectItem(ticket, "id")->valueint == ticketId){
      cJSON *update = NULL;
      // replaces cJSON item for every updatesJSON item
      cJSON_ArrayForEach(update, updatesJSON){
        cJSON_ReplaceItemInObject(ticket, update->string, cJSON_Duplicate(update, 1));
      }
      updated = 1;
      break;
    }
  }

  // doesn't change the file if it didn't find the right ticket
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
int assignTicketToAgent(int ticketId, User *agent){
  // creates updates JSON with new info
  cJSON *updates = cJSON_CreateObject();
  cJSON_AddStringToObject(updates, "agent", agent->username);
  cJSON_AddNumberToObject(updates, "state", IN_PROGRESS_STATE);

  // tries to update ticket
  if(updateTicket(ticketId, updates)){
    //!cJSON_Delete(updates);
    if(!setAgentStatus(agent->username, 0)){
      printf("Error changing agent status.\n");
      return 0;
    }
    printf("Ticket %d assigned to agent %s and state set to IN_PROGRESS_STATE.\n", ticketId, agent->username);
    return 1;
  }
  printf("Error: Ticket %d not found.\n", ticketId);
  return 0;
}

// returns if ticket is found
int getTicketById(int ticketId, Ticket *ticket){
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
  if(!cJSON_IsArray(ticketList)){
    printf("Error parsing ticket file.\n");
    return 0;
  }

  int found = 0;
  cJSON *ticketJSON = NULL;
  // goes through ticket list and returns if it finds the right one
  cJSON_ArrayForEach(ticketJSON, ticketList){
    if(cJSON_GetObjectItem(ticketJSON, "id")->valueint == ticketId){
      if(!parseJSONToTicket(ticketJSON, ticket)){
        printf("Error parsing ticket json.\n");
        return 0;
      }
      found = 1;
      break;
    }
  }

  //!cJSON_Delete(ticketList);
  return found;
}

// returns GENERAL_ERROR_CODE if there's a server-side error
// 0 if client has closed the connection, 1 otherwise
int handleMessage(int clientfd, char *stringMessage){
  Message message = {0};
  User userData = {0};

  if(parseJSONToMessage(cJSON_Parse(stringMessage), &message) == GENERAL_ERROR_CODE){
    printf("Error parsing message json.\n");
    return GENERAL_ERROR_CODE;
  }

  // authentication if user is not logging in, signing up, or closing the connection
  if(message.action_code!=LOGIN_REQUEST_MESSAGE_CODE && 
    message.action_code!=SIGNUP_MESSAGE_CODE &&
    message.action_code!=CLOSE_CONNECTION_MESSAGE_CODE){
    userData = authenticate(message.session_token);
    if(userData.role == GENERAL_ERROR_CODE){
      // if authentication fails, it informs client
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Invalid session, try logging in.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      return 1; // not an error server-side
    }
  }
  switch (message.action_code)
  {
    case CLOSE_CONNECTION_MESSAGE_CODE:{
      // sends client same message to inform it the connection is being closed server-side
      char *responseString = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseString, strlen(responseString));
      write(clientfd, "\0", 1);
      free(responseString);
      return 0;
    }
    case LOGIN_REQUEST_MESSAGE_CODE:{      
      if(parseJSONToUser(message.data, &userData)==GENERAL_ERROR_CODE){
        printf("ERROR: invalid json.\n");
        return GENERAL_ERROR_CODE;
      }
      printf("Client requested login for username %s.\n", userData.username);

      FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
      int fd = fileno(file);
      flock(fd, LOCK_SH);

      fseek(file, 0, SEEK_END);
      long fileSize = ftell(file);
      fseek(file, 0, SEEK_SET);

      char *fileContent = calloc(fileSize+1, sizeof(char));  

      int found = 0;
      cJSON *fileUsers = NULL;
      cJSON *fileUser = NULL;
      int index=-1;

      // if file isn't empty tries finding user with same username and password
      if(fileSize!=0){
        fread(fileContent, 1, fileSize, file);
        fileUsers = cJSON_Parse(fileContent);
        free(fileContent);
        
        cJSON_ArrayForEach(fileUser, fileUsers){
          index++;
          if(strcmp(cJSON_GetObjectItem(fileUser, "username")->valuestring, userData.username)==0){
            found = 1;
            break;
          }
        }

        // if user has been found
        if(found){
          // if password isn't correct informs client
          if(strcasecmp(cJSON_GetObjectItem(fileUser, "password")->valuestring, userData.password)!=0){
            message.action_code = INFO_MESSAGE_CODE;
            message.data = cJSON_CreateString("Invalid Password.");
            char *responseMessage = cJSON_PrintUnformatted(parseMessageToJSON(&message));
            write(clientfd, responseMessage, strlen(responseMessage));
            write(clientfd, "\0", 1);
            free(responseMessage);
            fclose(file);
            return 1;
          }
        }
      }
      
      if(!found){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Invalid username.");
        char *responseMessage = cJSON_PrintUnformatted(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        fclose(file);
        return 1;
      }

      // token generator
      char token[SESSION_TOKEN_LENGTH+1] = {0};
      int isUnique;
      do
      {
        srand(time(NULL)); // reset rand() otherwise it creates the same token
        for( int i = 0; i < SESSION_TOKEN_LENGTH; ++i){
          token[i] = '!' + rand()%93; // da 33(!) a 125 (})
        }

        // finds if there's another identical token in use
        isUnique = 1;
        cJSON *tempUser = NULL;
        cJSON_ArrayForEach(tempUser, fileUsers){
          if(strcmp(cJSON_GetObjectItem(tempUser, "token")->valuestring, token)==0){
            isUnique = 0;
            break;
          }
        }
      } while (!isUnique);

      // replaces token in cJSON
      cJSON_ReplaceItemInObject(fileUser, "token", cJSON_CreateString(token));
      cJSON_ReplaceItemInArray(fileUsers, index, fileUser);
      
      // updates file to save token
      ftruncate(fd, 0);
      fseek(file, 0, SEEK_SET);
      fileContent = cJSON_Print(fileUsers);
      fwrite(fileContent, 1, strlen(fileContent), file);
      fclose(file);

      // copies token in message.session_token to send it to client
      strcpy(message.session_token, token);

      message.action_code = LOGIN_INFO_MESSAGE_CODE;
      char *base = "Login successfull. Hi ";

      // string concatenation
      int fullStringLength = snprintf(NULL, 0, "%s%s.", base, userData.username);
      char *fullString = calloc(fullStringLength+1, sizeof(char));
      snprintf(fullString, fullStringLength+1, "%s%s.", base, userData.username);
      message.data = cJSON_CreateString(fullString);
      free(fullString);

      char *responseString = cJSON_PrintUnformatted(parseMessageToJSON(&message));
      write(clientfd, responseString, strlen(responseString));
      write(clientfd, "\0", 1);
      free(responseString);
      break;
    }
    case SIGNUP_MESSAGE_CODE:{
      User user = {0};
      if(parseJSONToUser(message.data, &user)==GENERAL_ERROR_CODE){
        printf("Error parsing user from message data.\n");
        return GENERAL_ERROR_CODE;
      }

      printf("Client requested account creation for username %s.\n", userData.username);

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

        if(!cJSON_IsArray(fileUsers)){
          printf("Error parsing users file.\n");
          cJSON_Delete(fileUsers);
          fclose(file);
          return GENERAL_ERROR_CODE;
        }
    
        cJSON *fileUser = NULL;

        // goes through users.json to see if the username is already taken
        cJSON_ArrayForEach(fileUser, fileUsers){
          if(strcmp(cJSON_GetObjectItem(fileUser, "username")->valuestring, user.username)==0){
            message.action_code = INFO_MESSAGE_CODE;
            message.data = cJSON_CreateString("The username is already taken.");
            char *messageString = cJSON_Print(parseMessageToJSON(&message));
            write(clientfd, messageString, strlen(messageString));
            write(clientfd, "\0", 1);
            free(messageString);
            return 1; // not an error server-side
          }
        }
      }

      user.role = USER_ROLE;
      user.isAvailable = 0;
      cJSON_AddItemToArray(fileUsers, parseUserToJSON(&user));

      // overrites file with updated user list
      ftruncate(fileno(file), 0);
      fseek(file, 0, SEEK_SET);
      char *newFileContent = cJSON_Print(fileUsers);
      fwrite(newFileContent, 1, strlen(newFileContent), file);
      fclose(file);
      free(newFileContent);

      // informs client of successfull registration
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("User created successfully.");
      char *messageString = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, messageString, strlen(messageString));
      write(clientfd, "\0", 1);
      free(messageString);
      break;
    }
    case CREATE_TICKET_MESSAGE_CODE:{
      printf("Client %s requested to create a ticket.\n", userData.username);

      // check if client is authenticated as user
      if(userData.role != USER_ROLE){
        message.data = cJSON_CreateString("Only an user can create a ticket.\n");
        message.action_code = INFO_MESSAGE_CODE;
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        return 1;
      }

      Ticket ticket = {0};
      if(!parseJSONToTicket(message.data, &ticket)){
        printf("ERROR: invalid json\n");
        return GENERAL_ERROR_CODE;
      }

      //server gets actual date for ticket date
      time_t actualTime = time(NULL);
      struct tm *timeInfo = localtime(&actualTime);
      ticket.date.day = timeInfo->tm_mday;
      ticket.date.month = timeInfo->tm_mon + 1; // tm_mon is 0-11
      ticket.date.year = timeInfo->tm_year + 1900; // tm_year is years since 1900

      // sets ticket user as authenticated user username
      strcpy(ticket.user, userData.username);
      
      // if an agent is available it assigns it the ticket
      int agentWasAssigned = 0;
      User assignedAgent;

      cJSON *availableAgents = loadAvailableAgents();
      if(!cJSON_IsArray(availableAgents)){
        printf("Error loading available agents.\n");
        return GENERAL_ERROR_CODE;
      }

      int agentsCount = cJSON_GetArraySize(availableAgents);
      
      if(agentsCount > 0){
        printf("Available agents found: %d\n", agentsCount);
        int nextAgentIndex = 0;
        cJSON *agentJSON = cJSON_GetArrayItem(availableAgents, nextAgentIndex % agentsCount);
        nextAgentIndex++;

        if(parseJSONToUser(agentJSON, &assignedAgent) == GENERAL_ERROR_CODE){
          printf("Error parsing agent json.\n");
          return GENERAL_ERROR_CODE;
        }

        agentWasAssigned = 1;
        printf("Agent %s assigned to ticket.\n", assignedAgent.username);
        ticket.state = IN_PROGRESS_STATE;
        strcpy(ticket.agent, assignedAgent.username);
      }

      // if there's no available agent the ticket remains OPEN
      if(!agentWasAssigned){
        printf("No available agents, ticket will be created with OPEN_STATE status.\n");
        ticket.state = OPEN_STATE;
        strcpy(ticket.agent, "N/A"); // no agent assigned
      }
      //!cJSON_Delete(availableAgents);

      // generates ticket ID and then saves it to ticketList.json
      ticket.id = getNextTicketId();
      cJSON *jsonTicket = parseTicketToJSON(&ticket);
      if(!saveTicket(jsonTicket)){
        printf("Error saving ticket.\n");
        return GENERAL_ERROR_CODE;
      }
      //! cJSON_Delete(jsonTicket);


      // if an agent has been assigned, updates agent status
      if(agentWasAssigned){
        if(!setAgentStatus(assignedAgent.username, 0)){
          printf("Error changing agent status.\n");
          return GENERAL_ERROR_CODE;
        }
        printf("Agent %s status set to busy.\n", assignedAgent.username);
      }

      // informs client
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
    case TICKET_CONSULTATION_MESSAGE_CODE:{
      printf("Client %s requested ticket consultation.\n", userData.username);

      // check if client is authenticated as user or agent
      if(userData.role != USER_ROLE && userData.role != AGENT_ROLE){
        message.data = cJSON_CreateString("Only an user or agent can consult their tickets.\n");
        message.action_code = INFO_MESSAGE_CODE;
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        return 1;
      }

      FILE *file = fopen(TICKET_FILE_ADDRESS, "r");
      flock(fileno(file), LOCK_SH);

      fseek(file, 0, SEEK_END);
      long fileSize = ftell(file);
      fseek(file, 0, SEEK_SET);
      
      // reads from file if it's not empty
      cJSON *returnTicketList = cJSON_CreateArray();
      if(fileSize!=0){
        char *fileContent = calloc(fileSize+1, sizeof(char));
        fread(fileContent, 1, fileSize, file);
        cJSON *ticketList = cJSON_Parse(fileContent);
        
        if(!cJSON_IsArray(ticketList)){
          printf("Error parsing ticket file.\n");
          return GENERAL_ERROR_CODE;
        }
        free(fileContent);

        cJSON *ticket = NULL;
        // finds every ticket with user=username of authenticated client
        cJSON_ArrayForEach(ticket, ticketList){
          if(strcmp(
              cJSON_GetObjectItem(ticket, userData.role==USER_ROLE? "user":"agent")->valuestring, 
                userData.username)==0){
            cJSON_AddItemToArray(returnTicketList, cJSON_Duplicate(ticket, 1));
          }
        }
      }

      // sends ticket list to client
      message.action_code = TICKET_CONSULTATION_MESSAGE_CODE;
      message.data = returnTicketList;
      char *responseString = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseString, strlen(responseString));
      write(clientfd, "\0", 1);
      fclose(file);
      break;
    }
    case RESOLVE_TICKET_MESSAGE_CODE:{
      printf("Client %s requested to resolve ticket.\n", userData.username);

      // check user is authenticated as agent
      if(userData.role!=AGENT_ROLE){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Only agents can resolve tickets.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        return 0; // not an error server-side
      }

      Ticket ticket = {0};
      if(parseJSONToTicket(message.data, &ticket)==GENERAL_ERROR_CODE){
        printf("Error parsing ticket json.\n");
        return GENERAL_ERROR_CODE;
      }    

      // finds ticket by id and puts it into "ticket"
      if(!getTicketById(ticket.id, &ticket)){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Ticket not found.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      // checks that ticket can be resolved
      if(ticket.state!=IN_PROGRESS_STATE){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("You can only resolve tickets which are 'in progress'.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      // checks that authenticated agent is the ticket agent
      if(strcmp(userData.username, ticket.agent)!=0){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("You can only resolve tickets assigned to you.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      // updates ticket state
      cJSON *stateUpdate = cJSON_CreateObject();
      cJSON_AddNumberToObject(stateUpdate, "state", CLOSED_STATE);
      updateTicket(ticket.id, stateUpdate);
      //!cJSON_Delete(stateUpdate);
      printf("Ticket %d resolved by agent %s.\n", ticket.id, userData.username);

      // tries to find another ticket to assign to agent
      Ticket nextTicket = {0};
      if(findBestOpenTicket(&nextTicket)){
        assignTicketToAgent(nextTicket.id, &userData);
      }
      // if there's no open ticket agent become available
      else{
        if(!setAgentStatus(userData.username, 1)){
          printf("Error changing agent status.\n");
          return GENERAL_ERROR_CODE;
        }
        printf("No open tickets available. Agent %s set to available.\n", userData.username);
      }
      
      //!cJSON_Delete(message.data);
      // informs client
      message.action_code = INFO_MESSAGE_CODE;
      message.data = cJSON_CreateString("Ticket resolved successfully.");
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }
    case UPDATE_TICKET_PRIORITY_MESSAGE_CODE:{
      printf("Client %s requested to update ticket.\n", userData.username);

      // checks that user is an admin
      if(userData.role != ADMIN_ROLE){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Only admins can update tickets.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      Ticket ticket = {0};
      if(parseJSONToTicket(message.data, &ticket)==GENERAL_ERROR_CODE){
        printf("Error parsing ticket json.\n");
        return GENERAL_ERROR_CODE;
      }

      // creates update JSON
      cJSON *updates = cJSON_CreateObject();
      cJSON_AddNumberToObject(updates, "priority", ticket.priority);

      // updates ticket priority (if it finds the right one)
      if(updateTicket(ticket.id, updates))
        message.data = cJSON_CreateString("Ticket updated successfully.");
      else
        message.data = cJSON_CreateString("Ticket not found.");
      
      // informs client if it was a success
      message.action_code = INFO_MESSAGE_CODE;
      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      break;
    }
    case ASSIGN_AGENT_MESSAGE_CODE:{
      printf("Client %s requested to assign new agent to ticket.\n", userData.username);

      // checks if user is an admin
      if(userData.role != ADMIN_ROLE){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Only admins can assign agents.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      Ticket newTicket = {0};
      if(parseJSONToTicket(message.data, &newTicket)==GENERAL_ERROR_CODE){
        printf("Error parsing ticket json.\n");
        return GENERAL_ERROR_CODE;
      }

      // tries to find ticket by id
      Ticket ticket = {0};
      if(!getTicketById(newTicket.id, &ticket)){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Invalid ticket ID. Please insert a correct ticket ID.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      // checks that it's not closed (you can't reassing an agent to a closed ticket)
      if(ticket.state == CLOSED_STATE){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("You can't reassign an agent to a ticket that has already been closed.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      // checks if agent is available
      cJSON *fileAgents = loadAvailableAgents();
      if(!cJSON_IsArray(fileAgents)){
        printf("Error loading available agents.\n");
        return GENERAL_ERROR_CODE;
      }

      cJSON *fileAgent = NULL;
      int isPresent = 0;
      cJSON_ArrayForEach(fileAgent, fileAgents){
        if(strcmp(cJSON_GetObjectItem(fileAgent, "username")->valuestring, newTicket.agent) == 0){
          isPresent = 1;
          break;
        }
      }
      if(!isPresent){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("You can't reassign an occupied agent.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      // creates update JSON with new agent (and state if necessary)
      cJSON *updates = cJSON_CreateObject();
      cJSON_AddStringToObject(updates, "agent", newTicket.agent);

      // if state is in_progress it doesn't have to be updated
      if(ticket.state == OPEN_STATE)
        cJSON_AddNumberToObject(updates, "state", IN_PROGRESS_STATE);

      // updates ticket
      if(updateTicket(ticket.id, updates)){
        // if the ticket was assigned to another agent (wasn't open) sets old agent to available
        if(ticket.state == IN_PROGRESS_STATE){
          if(!setAgentStatus(ticket.agent, 1)){
            printf("Error changing agent status.\n");
            return GENERAL_ERROR_CODE;
          }
        }

        // sets new agent to unavailable
        if(!setAgentStatus(newTicket.agent, 0)){
          printf("Error changing agent status.\n");
          return GENERAL_ERROR_CODE;
        }

        // informs client
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Agent assigned successfully.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        return 1;
      }
      return GENERAL_ERROR_CODE;
    }
    case CREATE_AGENT_MESSAGE_CODE:{
      printf("Client %s requested to create new agent.\n", userData.username);

      //the user must be an admin
      if(userData.role != ADMIN_ROLE){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Only admins can create agents.");
        char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
        write(clientfd, responseMessage, strlen(responseMessage));
        write(clientfd, "\0", 1);
        free(responseMessage);
        break; // not an error server-side
      }

      User newAgent = {0};
      if(parseJSONToUser(message.data, &newAgent)==GENERAL_ERROR_CODE){
        printf("Error parsing user from message data.\n");
        return GENERAL_ERROR_CODE;
      }

      newAgent.role = AGENT_ROLE; // force role to agent

      FILE *file = fopen(USERS_FILE_ADDRESS, "r+");
      flock(fileno(file), LOCK_EX);

      fseek(file, 0, SEEK_END);
      long fileSize = ftell(file);
      fseek(file, 0, SEEK_SET);

      //control if username is already taken
      cJSON *fileUsers = cJSON_CreateArray();

      //load existing users
      if(fileSize!=0){
        char *fileContent = calloc(fileSize+1, sizeof(char));
        fread(fileContent, 1, fileSize, file);
        fileUsers = cJSON_Parse(fileContent);
        free(fileContent);
        if(!cJSON_IsArray(fileUsers)){
          printf("Error parsing users file.\n");
          cJSON_Delete(fileUsers);
          fclose(file);
          return GENERAL_ERROR_CODE;
        }
      }

      // check username uniqueness
      int usernameTaken = 0;
      cJSON *user = NULL;
      cJSON_ArrayForEach(user, fileUsers){
        if(strcmp(cJSON_GetObjectItem(user, "username")->valuestring, newAgent.username)==0){
          usernameTaken = 1;
          break;
        }
      }
      if(usernameTaken){
        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("The username is already taken.");
      } 
      else {
        newAgent.isAvailable = 1; // new agents are available by default
        cJSON_AddItemToArray(fileUsers, parseUserToJSON(&newAgent));

        // overrites file with updated file content
        ftruncate(fileno(file), 0);
        fseek(file, 0, SEEK_SET);
        char *newFileContent = cJSON_Print(fileUsers);
        fwrite(newFileContent, 1, strlen(newFileContent), file);
        free(newFileContent);

        message.action_code = INFO_MESSAGE_CODE;
        message.data = cJSON_CreateString("Agent created successfully.");
      }

      char *responseMessage = cJSON_Print(parseMessageToJSON(&message));
      write(clientfd, responseMessage, strlen(responseMessage));
      write(clientfd, "\0", 1);
      free(responseMessage);
      cJSON_Delete(fileUsers);
      fclose(file);
      break; 
    }
    default:{
      printf("ERROR: Invalid action_code.\n");
      return GENERAL_ERROR_CODE;
    }
  }
  return 1;
}

int main(int argc, char *argv[]){
  int socketfd, clientfd;
  socklen_t clientAddressSize;
  struct sockaddr_in serverAddress, clientAddress;

  // creates data folder if it doesn't exists
  mkdir(DATA_FOLDER_ADDRESS, 0777);

  // creates files if they don't exist
  FILE *file = fopen(TICKET_FILE_ADDRESS, "a");
  fclose(file);
  file = fopen(USERS_FILE_ADDRESS, "a");
  fclose(file);

  signal(SIGCHLD, SIG_IGN); // anti zombie handler

  // socket creation
  if ((socketfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("Error creating socket");
    exit(-1);
  }
  printf("Server socket created successfully.\n");

  // keeps socket open after killing server process
  int reuse=1;
  if(setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, (void *)&reuse, sizeof(reuse))<0){
    perror("Errore SO_REUSEADDR");
  }

  // server address definition
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
        
        // keeps connection open until close_connection_signal
        while(1){
          char receivedMessage[256]= {0};
          // if socket gets closed (bytes read are 0) breaks connection
          if(readMessage(clientfd, receivedMessage)==0){
            printf("Client %s:%d has interrupted the connection abnormally.\n", 
              inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            break;
          }

          // handle message is 0 when client requested to close the connection
          int handleMessageCode = handleMessage(clientfd, receivedMessage);
          if(handleMessageCode == 0){
            printf("Client %s:%d has requested to close the connection.\n", 
              inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
            break;
          }
          // error handling (informs client of error but doesn't stop server)
          else if(handleMessageCode == GENERAL_ERROR_CODE) {
            Message message = {0};
            message.action_code = INFO_MESSAGE_CODE;
            message.data = cJSON_CreateString("The server encountered an error, please refer to the system admin.\n");
            char *responseString = cJSON_Print(parseMessageToJSON(&message));
            write(clientfd, responseString, strlen(responseString));
            write(clientfd, "\0", 1);
            free(responseString);
          }
          
          printf("Request handled.\n");
        }
        close(clientfd);
        printf("Connection closed.\n");
        exit(0); // terminate child process
      }
    }
  }
  
  return 0;
}