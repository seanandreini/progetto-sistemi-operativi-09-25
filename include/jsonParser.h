#include "../lib/cJSON/cJSON.h"
#include "ticket.h"
#include "message.h"

cJSON *parseTicketToJSON(Ticket *ticket);
cJSON *parseDateToJSON(Date *date);
cJSON *parseAgentToJSON(Agent *agent);
void parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket);
cJSON *parseMessageToJSON(Message *message);
short parseJSONToMessage(cJSON *jsonMessage, Message *message);
cJSON *parseLoginDataToJSON(LoginData *loginData);
short parseJSONToLoginData(cJSON *jsonLoginData, LoginData *loginData);