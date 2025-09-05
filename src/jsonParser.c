#include "../include/jsonParser.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

cJSON *parseTicketToJSON(Ticket *ticket) {
  cJSON *jsonTicket = cJSON_CreateObject();

  cJSON_AddNumberToObject(jsonTicket, "id", ticket->id);
  cJSON_AddStringToObject(jsonTicket, "title", ticket->title);
  cJSON_AddStringToObject(jsonTicket, "description", ticket->description);

  cJSON *jsonDate = parseDateToJSON(&ticket->date);
  cJSON_AddItemToObject(jsonTicket, "date", jsonDate);

  cJSON_AddNumberToObject(jsonTicket, "priority", ticket->priority);
  cJSON_AddNumberToObject(jsonTicket, "state", ticket->state);
  cJSON_AddStringToObject(jsonTicket, "user", ticket->user);
  cJSON_AddStringToObject(jsonTicket, "agent", ticket->agent);

  return jsonTicket;
}
int parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket) {
  cJSON *id = cJSON_GetObjectItem(jsonTicket, "id");
  if(id != NULL)
    ticket->id = id->valueint;

  strncpy(ticket->title, cJSON_GetObjectItem(jsonTicket, "title")->valuestring, sizeof(ticket->title)-1);
  ticket->title[sizeof(ticket->title)-1] = '\0';

  cJSON *user = cJSON_GetObjectItem(jsonTicket, "user");

  if(user != NULL)
    strncpy(ticket->user, user->valuestring, MAX_USERNAME_LENGTH);

  strncpy(ticket->description, cJSON_GetObjectItem(jsonTicket, "description")->valuestring, sizeof(ticket->description)-1);
  
  cJSON *jsonDate = cJSON_GetObjectItem(jsonTicket, "date");
  parseJSONToDate(jsonDate, &ticket->date);
  
  ticket->priority = (Priority)cJSON_GetObjectItem(jsonTicket, "priority")->valueint;
  ticket->state = (State)cJSON_GetObjectItem(jsonTicket, "state")->valueint;
  
  strncpy(ticket->user, cJSON_GetObjectItem(jsonTicket, "user")->valuestring, sizeof(ticket->user)-1);
  strncpy(ticket->agent, cJSON_GetObjectItem(jsonTicket, "agent")->valuestring, sizeof(ticket->agent)-1);
  
  return 1;
}
cJSON *parseDateToJSON(Date *date) {
  cJSON *jsonDate = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonDate, "day", date->day);
  cJSON_AddNumberToObject(jsonDate, "month", date->month);
  cJSON_AddNumberToObject(jsonDate, "year", date->year);
  return jsonDate;
}
int parseJSONToDate(cJSON *jsonDate, Date *date){
  if(jsonDate==NULL) return GENERAL_ERROR_CODE;

  date->day = cJSON_GetObjectItem(jsonDate, "day")->valueint;
  date->month = cJSON_GetObjectItem(jsonDate, "month")->valueint;
  date->year = cJSON_GetObjectItem(jsonDate, "year")->valueint;

  return 1;
}
cJSON *parseAgentToJSON(Agent *agent) {
  cJSON *jsonAgent = cJSON_CreateObject();
  cJSON_AddStringToObject(jsonAgent, "username", agent->username);
  cJSON_AddNumberToObject(jsonAgent, "isAvailable", agent->isAvailable);
  return jsonAgent;
}
int parseJSONToAgent(cJSON *jsonAgent, Agent *agent){
  if(jsonAgent==NULL) return GENERAL_ERROR_CODE;

  strncpy(agent->username, cJSON_GetObjectItem(jsonAgent, "username")->valuestring, sizeof(agent->username)-1);
  agent->username[sizeof(agent->username)-1] = '\0';
  agent -> isAvailable = cJSON_GetObjectItem(jsonAgent, "isAvailable")->valueint;

  return 1;
}
cJSON *parseMessageToJSON(Message *message){
  cJSON *jsonMessage = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonMessage, "action_code", message->action_code);
  cJSON_AddStringToObject(jsonMessage, "session_token", message->session_token);
  cJSON_AddItemToObject(jsonMessage, "data", message->data);

  return jsonMessage;
}
int parseJSONToMessage(cJSON *jsonMessage, Message *message){
  if(jsonMessage==NULL) return GENERAL_ERROR_CODE;
  message->action_code = cJSON_GetObjectItem(jsonMessage, "action_code")->valueint;
  strcpy(message->session_token, cJSON_GetObjectItem(jsonMessage, "session_token")->valuestring);
  message->data = cJSON_GetObjectItem(jsonMessage, "data");

  return 1;
}
cJSON *parseUserToJSON(User *user){
  cJSON *jsonUser = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonUser, "role", user->role);
  cJSON_AddStringToObject(jsonUser, "username", user->username);
  cJSON_AddStringToObject(jsonUser, "password", user->password);
  cJSON_AddStringToObject(jsonUser, "token", user->token);
  cJSON_AddNumberToObject(jsonUser, "isAvailable", user->isAvailable);

  return jsonUser;
}
int parseJSONToUser(cJSON *jsonUser, User *user){
  if(jsonUser==NULL) return GENERAL_ERROR_CODE;

  user->role = cJSON_GetObjectItem(jsonUser, "role")->valueint;
  strcpy(user->username, cJSON_GetObjectItem(jsonUser, "username")->valuestring);
  strcpy(user->password, cJSON_GetObjectItem(jsonUser, "password")->valuestring);
  strcpy(user->token, cJSON_GetObjectItem(jsonUser, "token")->valuestring);
  user->isAvailable = cJSON_GetObjectItem(jsonUser, "isAvailable")->valueint;

  return 1;
}