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

  cJSON *jsonAgent = parseAgentToJSON(&ticket->agent);
  cJSON_AddItemToObject(jsonTicket, "agent", jsonAgent);

  return jsonTicket;
}

cJSON *parseDateToJSON(Date *date) {
  cJSON *jsonDate = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonDate, "giorno", date->day);
  cJSON_AddNumberToObject(jsonDate, "mese", date->month);
  cJSON_AddNumberToObject(jsonDate, "anno", date->year);
  return jsonDate;
}

cJSON *parseAgentToJSON(Agent *agent) {
  cJSON *jsonAgent = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonAgent, "code", agent->code);
  cJSON_AddStringToObject(jsonAgent, "username", agent->username);
  cJSON_AddNumberToObject(jsonAgent, "isAvailable", agent->isAvailable);
  return jsonAgent;
}

int parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket) {
  if(jsonTicket==NULL) return 0;

  ticket->id = cJSON_GetObjectItem(jsonTicket, "id")->valueint;

  strncpy(ticket->title, cJSON_GetObjectItem(jsonTicket, "title")->valuestring, sizeof(ticket->title)-1);
  ticket->title[sizeof(ticket->title)-1] = '\0';

  strncpy(ticket->description, cJSON_GetObjectItem(jsonTicket, "description")->valuestring, sizeof(ticket->description)-1);
  ticket->description[sizeof(ticket->description)-1] = '\0';

  cJSON *jsonDate = cJSON_GetObjectItem(jsonTicket, "date");
  ticket->date.day = cJSON_GetObjectItem(jsonDate, "giorno")->valueint;
  ticket->date.month = cJSON_GetObjectItem(jsonDate, "mese")->valueint;
  ticket->date.year = cJSON_GetObjectItem(jsonDate, "anno")->valueint;
  parseJSONToDate(jsonDate, &ticket->date);

  ticket->priority = (Priority)cJSON_GetObjectItem(jsonTicket, "priority")->valueint;
  ticket->state = (State)cJSON_GetObjectItem(jsonTicket, "state")->valueint;

  cJSON *jsonAgent = cJSON_GetObjectItem(jsonTicket, "agent");
  parseJSONToAgent(jsonAgent, &ticket->agent);
  
  return 1;
}

cJSON *parseDateToJSON(Date *date) {
  cJSON *jsonDate = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonDate, "giorno", date->day);
  cJSON_AddNumberToObject(jsonDate, "mese", date->month);
  cJSON_AddNumberToObject(jsonDate, "anno", date->year);
  return jsonDate;
}

cJSON *parseAgentToJSON(Agent *agent) {
  cJSON *jsonAgent = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonAgent, "code", agent->code);
  cJSON_AddStringToObject(jsonAgent, "username", agent->username);
  return jsonAgent;
}

cJSON *parseMessageToJSON(Message *message){
  cJSON *jsonMessage = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonMessage, "action_code", message->action_code);
  cJSON_AddStringToObject(jsonMessage, "session_token", message->session_token);
  cJSON_AddItemToObject(jsonMessage, "data", message->data);

  return jsonMessage;
}

int parseJSONToMessage(cJSON *jsonMessage, Message *message){
  if(jsonMessage==NULL) return 0;
  message->action_code = cJSON_GetObjectItem(jsonMessage, "action_code")->valueint;
  strcpy(message->session_token, cJSON_GetObjectItem(jsonMessage, "session_token")->valuestring);
  message->data = cJSON_GetObjectItem(jsonMessage, "data");

  return 1;
}

cJSON *parseLoginDataToJSON(LoginData *loginData){
  cJSON *jsonLoginData = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonLoginData, "request_type", loginData->request_type);
  cJSON_AddNumberToObject(jsonLoginData, "role", loginData->role);
  cJSON_AddStringToObject(jsonLoginData, "username", loginData->username);
  cJSON_AddStringToObject(jsonLoginData, "password", loginData->password);
  cJSON_AddStringToObject(jsonLoginData, "token", loginData->token);

  return jsonLoginData;
}

int parseJSONToLoginData(cJSON *jsonLoginData, LoginData *loginData){
  if(jsonLoginData==NULL) return 0;

  // loginData->request_type = cJSON_GetObjectItem(jsonLoginData, "request_type")->valueint;
  loginData->role = cJSON_GetObjectItem(jsonLoginData, "role")->valueint;
  strcpy(loginData->username, cJSON_GetObjectItem(jsonLoginData, "username")->valuestring);
  strcpy(loginData->password, cJSON_GetObjectItem(jsonLoginData, "password")->valuestring);
  strcpy(loginData->token, cJSON_GetObjectItem(jsonLoginData, "token")->valuestring);

  return 1;
}

int parseJSONToAgent(cJSON *jsonAgent, Agent *agent){  //??migliorabile??
  if(jsonAgent==NULL) return 0;

  agent->code = cJSON_GetObjectItem(jsonAgent, "code")->valueint;
  strncpy(agent->username, cJSON_GetObjectItem(jsonAgent, "username")->valuestring, sizeof(agent->username)-1);
  agent->username[sizeof(agent->username)-1] = '\0';
  agent -> isAvailable = cJSON_GetObjectItem(jsonAgent, "isAvailable")->valueint;

  return 1;
}

int parseJSONToDate(cJSON *jsonDate, Date *date){
  if(jsonDate==NULL) return 0;

  date->day = cJSON_GetObjectItem(jsonDate, "giorno")->valueint;
  date->month = cJSON_GetObjectItem(jsonDate, "mese")->valueint;
  date->year = cJSON_GetObjectItem(jsonDate, "anno")->valueint;

  return 1;
}