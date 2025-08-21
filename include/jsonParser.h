#include "../lib/cJSON/cJSON.h"
#include "ticket.h"

cJSON *parseTicketToJSON(Ticket *ticket) ;
cJSON *parseDateToJSON(Date *date);
cJSON *parseAgentToJSON(Agent *agent);
void parseJSONToTicket(cJSON *jsonTicket, Ticket *ticket);