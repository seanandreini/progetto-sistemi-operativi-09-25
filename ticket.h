#ifndef TICKET_H
#define TICKET_H

#include "agent.h"

typedef struct struct_data{
    int giorno;
    int mese;
    int anno;
} data;

typedef enum Priority{
    LOW = 1,
    MEDIUM = 2,
    HIGH = 3,
    MIN_PRIORITY = LOW,
    MAX_PRIORITY = HIGH
} Priority;

typedef enum State{
    OPEN = 1,
    IN_PROGRESS = 2,
    CLOSED = 3,
    MIN_STATE = OPEN,
    MAX_STATE = CLOSED
} State;

typedef struct struct_ticket{
    int id; // unique identifier for the ticket //!(scegliere come fare, il server dovrà generarlo)
    char title[50];
    char description[200];
    data date;
    Priority priority;
    State state;
    agent agent;  
} ticket;

#endif 
