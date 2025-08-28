#include "../lib/cJSON/cJSON.h"
#include "ticket.h"
#include "message.h"

cJSON *parseTicketToJSON(Ticket *ticket);
cJSON *parseDateToJSON(Date *date);
cJSON *parseAgentToJSON(Agent *agent);
int parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket);
cJSON *parseMessageToJSON(Message *message);
int parseJSONToMessage(cJSON *jsonMessage, Message *message);
cJSON *parseLoginDataToJSON(LoginData *loginData);
int parseJSONToLoginData(cJSON *jsonLoginData, LoginData *loginData);
int parseJSONToAgent(cJSON *jsonAgent, Agent *agent);