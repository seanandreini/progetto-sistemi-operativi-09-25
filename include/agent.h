#ifndef AGENT_H
#define AGENT_H

typedef struct struct_agent{
    int code; // unique identifier for the agent //!(scegliere come fare)
    char username[50];
    int isAvailable; // 1 if available, 0 if busy
} Agent;

#endif 

//creo a mano gli agenti
//funzioni per il parse del file json
//funzione che assegna ticket ad agente sul server (logica si farà più avanti)
