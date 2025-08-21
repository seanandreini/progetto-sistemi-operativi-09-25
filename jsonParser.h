#include "cJSON.h"
#include "ticket.h"

cJSON *parseTicketToJSON(Ticket *ticket) ;
cJSON *parseDateToJSON(Date *date);
cJSON *parseAgentToJSON(Agent *agent);
void parseJSONToTicket(Ticket *ticket, cJSON *jsonTicket);