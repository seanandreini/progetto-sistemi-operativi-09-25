#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "../include/ticket.h"
#include "../lib/cJSON/cJSON.h"
#include "../include/jsonParser.h"
#include "../include/functions.h"
#include "../include/message.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"

//! USARE FGETS INVECE CHE SCANF O PULIRE IL BUFFER

Ticket createTicket() {
  Ticket t;
  int goodInput = 0;

  printf("Inserisci i dati del ticket:\n");
  printf("Titolo:\n");
  fgets(t.title, sizeof(t.title), stdin); 
  printf("descrizione:\n");
  fgets(t.description, sizeof(t.description), stdin);

  //TODO: inserire logica controllo data
  printf("Data (gg/mm/aaaa):\n");
  scanf("%d/%d/%d", &t.date.giorno, &t.date.mese, &t.date.anno);

  while(!goodInput) {
    int temp_priority;
    printf("Priorità (1: Bassa, 2: Media, 3: Alta):\n");
    scanf("%d", &temp_priority);
    if (t.priority < MIN_PRIORITY || t.priority > MAX_PRIORITY) {
      printf("Priorità non valida, riprova.\n");
      continue;
    }
    goodInput = 1;
    t.priority = temp_priority; 
  }

  goodInput = 0;
  while(!goodInput) {
    int temp_state;
    printf("Stato (1: Aperto, 2: In Corso, 3: Chiuso):\n");
    scanf("%d", &temp_state);
    if(temp_state < MIN_STATE || temp_state > MAX_STATE){
      printf("Stato non valido, riprova.\n");
      continue;
    }
    goodInput = 1;
    t.state = temp_state;
  }

  return t;
}


// message interpretation
void handleMessage(char *stringMessage, char *sessionToken){
  Message message;
  if(!parseJSONToMessage(cJSON_Parse(stringMessage), &message)){
    printf("No response received.\n");
    return;
  }

  // cJSON *jsonInData = cJSON_GetObjectItem(jsonInMessage, "data");

  switch (message.action_code)
  {
  case MESSAGE_CODE:{
    printf("Server: %s\n", cJSON_Print(message.data));
    break;
  }

  case LOGIN_INFO_CODE:{
    // char *message = cJSON_GetObjectItem(jsonInData, "message")->valuestring;
    // sessionToken = cJSON_GetObjectItem(jsonInData, "token")->valuestring;
    LoginData loginData;
    parseJSONToLoginData(message.data, &loginData);
    printf("Token: %s\n", loginData.token);
    break;
  }
  
  default:{
    printf("ERROR: Invalid action_code.\n");
    break;
  }
  }
}

int main(int argc, char *argv[]){
  printf("Client started.\n");
  int clientfd, resultCode;
  struct sockaddr_in serverAddress;
  // create socket
  clientfd = socket(AF_INET, SOCK_STREAM, 0);

  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(SERVER_PORT);
  serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

  // connection
  do
  {
    resultCode = connect(clientfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if(resultCode == -1) {
      printf("Error connecting to server, trying again in 1 second\n");
      sleep(1);
    }
  } while (resultCode == -1);

  printf("Connected to server at %s:%d\n", SERVER_ADDRESS, SERVER_PORT);


  char sessionToken[SESSION_TOKEN_LENGTH];
  Message message;


  //* CREAZIONE TICKET 
  // Ticket ticket;
  // strncpy(ticket.title, "Ticket Titolo", strlen("Ticket Titolo"));
  // strncpy(ticket.description, "Ticket description", strlen("Ticket description"));
  // ticket.date.giorno = 1;
  // ticket.date.mese = 1;
  // ticket.date.anno = 2023;
  // ticket.priority = MEDIUM;
  // ticket.state = OPEN;
  // message.action_code = CREATE_TICKET_CODE;
  // message.data = parseTicketToJSON(&ticket);


  //* RICHIESTA LOGIN
  // Message message;
  // message.action_code = LOGIN_REQUEST_CODE;
  // LoginData loginData;
  // loginData.request_type = LOGIN_REQUEST;
  // strcpy(loginData.username, "sean");
  // strcpy(loginData.password, "password");
  // message.data = parseLoginDataToJSON(&loginData);

  // writing to server
  char *stringMessage = cJSON_Print(parseMessageToJSON(&message));
  write(clientfd, stringMessage, strlen(stringMessage));
  write(clientfd, "\0", 1);
  printf("Message sent to server.\n");

  // client receiving
  char receivedMessage[256]= {0};
  readMessage(clientfd, receivedMessage);
  // printf("received %s\n", receivedMessage);
  handleMessage(receivedMessage, sessionToken);



  //! TESTING MULTIPLE CONNECTIONS, DELETE LATER
  // sleep(10);


  // close socket
  close(clientfd);
  printf("Connection closed.\n");


  exit(0);
}