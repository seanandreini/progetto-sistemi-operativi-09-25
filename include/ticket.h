#ifndef TICKET_H
#define TICKET_H

#include "agent.h"
#include "message.h"

typedef struct struct_date{
    int day;
    int month;
    int year;
} Date;

typedef enum Priority{
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    MIN_PRIORITY = LOW,
    MAX_PRIORITY = HIGH
} Priority;

typedef enum State{
    OPEN_STATE = 1,
    IN_PROGRESS_STATE = 2,
    CLOSED_STATE = 3,
    MIN_STATE = OPEN_STATE,
    MAX_STATE = CLOSED_STATE
} State;

typedef struct struct_ticket{
    int id; // unique identifier for the ticket //!(scegliere come fare, il server dovrà generarlo)
    char title[50];
    char user[MAX_USERNAME_LENGTH+1];
    char description[200];
    Date date;
    Priority priority;
    State state;
    char agent[MAX_USERNAME_LENGTH+1];  
} Ticket;

#endif 
