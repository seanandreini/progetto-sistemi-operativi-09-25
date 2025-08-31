#include "../lib/cJSON/cJSON.h"
#include "ticket.h"
#include "message.h"

cJSON *parseTicketToJSON(Ticket *ticket);
int parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket);
cJSON *parseDateToJSON(Date *date);
int parseJSONToDate(cJSON *jsonDate, Date *date);
cJSON *parseAgentToJSON(Agent *agent);
int parseJSONToAgent(cJSON *jsonAgent, Agent *agent);
cJSON *parseMessageToJSON(Message *message);
int parseJSONToMessage(cJSON *jsonMessage, Message *message);
// cJSON *parseLoginDataToJSON(LoginData *loginData);
// int parseJSONToLoginData(cJSON *jsonLoginData, LoginData *loginData);
cJSON *parseUserToJSON(User *user);
int parseJSONToUser(cJSON *jsonUser, User *user);