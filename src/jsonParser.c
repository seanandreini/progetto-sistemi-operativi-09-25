#include "../include/jsonParser.h"
#include <string.h>

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
  cJSON_AddNumberToObject(jsonDate, "giorno", date->giorno);
  cJSON_AddNumberToObject(jsonDate, "mese", date->mese);
  cJSON_AddNumberToObject(jsonDate, "anno", date->anno);
  return jsonDate;
}

cJSON *parseAgentToJSON(Agent *agent) {
  cJSON *jsonAgent = cJSON_CreateObject();
  cJSON_AddNumberToObject(jsonAgent, "code", agent->code);
  cJSON_AddStringToObject(jsonAgent, "username", agent->username);
  return jsonAgent;
}

void parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket) {
  ticket->id = cJSON_GetObjectItem(jsonTicket, "id")->valueint;

  //! SE PROBLEMI USARE MALLOC
  strncpy(ticket->title, cJSON_GetObjectItem(jsonTicket, "title")->valuestring, sizeof(ticket->title)-1);
  ticket->title[sizeof(ticket->title)-1] = '\0';

  strncpy(ticket->description, cJSON_GetObjectItem(jsonTicket, "description")->valuestring, sizeof(ticket->description)-1);
  ticket->description[sizeof(ticket->description)-1] = '\0';

  cJSON *jsonDate = cJSON_GetObjectItem(jsonTicket, "date");
  ticket->date.giorno = cJSON_GetObjectItem(jsonDate, "giorno")->valueint;
  ticket->date.mese = cJSON_GetObjectItem(jsonDate, "mese")->valueint;
  ticket->date.anno = cJSON_GetObjectItem(jsonDate, "anno")->valueint;

  ticket->priority = (Priority)cJSON_GetObjectItem(jsonTicket, "priority")->valueint;
  ticket->state = (State)cJSON_GetObjectItem(jsonTicket, "state")->valueint;

  cJSON *jsonAgent = cJSON_GetObjectItem(jsonTicket, "agent");
  ticket->agent.code = cJSON_GetObjectItem(jsonAgent, "code")->valueint;
  strncpy(ticket->agent.username, cJSON_GetObjectItem(jsonAgent, "username")->valuestring, sizeof(ticket->agent.username)-1);
  ticket->agent.username[sizeof(ticket->agent.username)-1] = '\0';
}