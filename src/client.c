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
#include <time.h>
#include "../include/ticket.h"
#include "../lib/cJSON/cJSON.h"
#include "../include/jsonParser.h"
#include "../include/functions.h"
#include "../include/message.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"

//! USARE FGETS INVECE CHE SCANF O PULIRE IL BUFFER
//! Alcune volte si usa italiano altre inglese, forse meglio se lo fai te che il mio inglese... -_- !

Ticket createTicket() {
  Ticket ticket;
  int goodInput = 0;

  printf("Inserisci i dati del ticket:\n");
  printf("Titolo:\n");
  fgets(ticket.title, sizeof(ticket.title), stdin); 
  ticket.title[strcspn(ticket.title, "\n")] = 0; // remove newline character

  printf("descrizione:\n");
  fgets(ticket.description, sizeof(ticket.description), stdin);
  ticket.description[strcspn(ticket.description, "\n")] = 0; // remove newline character

  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  int currentDay = tm.tm_mday;
  int currentMonth = tm.tm_mon + 1; // tm_mon is 0-11
  int currentYear = tm.tm_year + 1900; // tm_year is years since 1900

  while (!goodInput) {
    int day, month, year;
    printf("Data (gg/mm/aaaa):\n");
    
    // Check scanf return value
    if(scanf("%d/%d/%d", &day, &month, &year) != 3) {
      printf("Formato data non valido, riprova.\n");
      while(getchar() != '\n'); // clear wrong input buffer
      continue;
    }
    while(getchar() != '\n'); // clear input buffer
    
    // Check for the month days
    int daysInMonth = 31;
    if(month == 4 || month == 6 || month == 9 || month == 11) {
      daysInMonth = 30;
    } else if(month == 2) {
      // Check for leap year
      if((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
        daysInMonth = 29;
      } else {
        daysInMonth = 28;
      }
    }
    
    if(year < currentYear || 
       year > currentYear || // allow tickets only for the current year
       month < 1 || month > 12 || 
       day < 1 || day > daysInMonth ||
       (year == currentYear && month < currentMonth) ||
       (year == currentYear && month == currentMonth && day < currentDay)) {
      printf("Data non valida, riprova.\n");
      continue;
    }

    goodInput = 1;
    ticket.date.day = day;
    ticket.date.month = month;
    ticket.date.year = year;
  }
  
  goodInput = 0; // reset for next input
  
  while(!goodInput) {
    int temp_priority;
    printf("Priorità (1: Bassa, 2: Media, 3: Alta):\n");
    scanf("%d", &temp_priority);
    while(getchar() != '\n'); // clear input buffer
    if (temp_priority < MIN_PRIORITY || temp_priority > MAX_PRIORITY) {
      printf("Priorità non valida, riprova.\n");
      continue;
    }
    goodInput = 1;
    ticket.priority = temp_priority; 
  }

  goodInput = 0;
  while(!goodInput) {
    int temp_state;
    printf("Stato (1: Aperto, 2: In Corso, 3: Chiuso):\n");
    scanf("%d", &temp_state);
    while(getchar() != '\n'); // clear input buffer
    if(temp_state < MIN_STATE || temp_state > MAX_STATE){
      printf("Stato non valido, riprova.\n");
      continue;
    }
    goodInput = 1;
    ticket.state = temp_state;
  }

  return ticket;
}

// ask user for input credentials
void getUserCredentials(User *user){
  printf("Enter username: ");
  fgets(user->username, sizeof(user->username), stdin);
  user->username[strcspn(user->username, "\n")] = 0; // remove newline character

  printf("Enter password: ");
  fgets(user->password, sizeof(user->password), stdin);
  user->password[strcspn(user->password, "\n")] = 0; // remove newline character
}

// ask user for id of ticket to resolve
void getTicketIdToResolve(Ticket *ticket){
  printf("Enter ticket id to resolve: ");
  scanf("%d", &ticket->id); // !! se vogliamo usare fgets poi dobbiamo convertire in int usando atoi, che facciamo ??
  while(getchar() != '\n'); // clear input buffer
}

// ask admin the username and password of the agent to create
void getAgentData(User *agent){
  printf("Enter agent username: ");
  fgets(agent->username, sizeof(agent->username), stdin);
  agent->username[strcspn(agent->username, "\n")] = 0; // remove newline character

  printf("Enter agent password: ");
  fgets(agent->password, sizeof(agent->password), stdin);
  agent->password[strcspn(agent->password, "\n")] = 0; // remove newline character
}

//!! solo l'admin può aggiornare la priorità ??
// ask admin the new ticket priority 
void getTicketDataToUpdate(cJSON *updatesJSON){
  int goodInput = 0;
  while(!goodInput) {
    int temp_priority;
    printf("Enter new ticket priority (1: Low, 2: Medium, 3: High):\n");
    scanf("%d", &temp_priority); 
    while(getchar() != '\n'); // clear input buffer
    if (temp_priority < MIN_PRIORITY || temp_priority > MAX_PRIORITY) {
      printf("Invalid priority, try again.\n");
      continue;
    }
    goodInput = 1;
    cJSON_AddNumberToObject(updatesJSON, "priority", temp_priority); 
  }
}

// ask admin the new ticket agent
void getTicketAgentToUpdate(cJSON *updatesJSON){
  char agentUsername[MAX_USERNAME_LENGTH+1];
  printf("Enter new ticket agent username: ");
  fgets(agentUsername, sizeof(agentUsername), stdin);
  agentUsername[strcspn(agentUsername, "\n")] = 0; // remove newline character

  cJSON *agentObj = cJSON_CreateObject();
  cJSON_AddStringToObject(agentObj, "username", agentUsername); 
  cJSON_AddItemToObject(updatesJSON, "agent", agentObj); //add the agent object to the updates
  //!! dobbiamo scegliere se fare cosi o solo con la stringa dell'username !!
}

// message interpretation
void handleMessage(char *stringMessage, User *userData){
  Message message = {0};
  if(!parseJSONToMessage(cJSON_Parse(stringMessage), &message)){
    printf("Invalid json response.\n");
    return;
  }

  // cJSON *jsonInData = cJSON_GetObjectItem(jsonInMessage, "data");

  switch (message.action_code)
  {
  case INFO_MESSAGE_CODE:{
    printf("Server: %s\n", cJSON_Print(message.data));
    break;
  }

  case LOGIN_INFO_MESSAGE_CODE:{
    // char *message = cJSON_GetObjectItem(jsonInData, "message")->valuestring;
    // sessionToken = cJSON_GetObjectItem(jsonInData, "token")->valuestring;
    if(strlen(message.session_token)!=0){
      printf("Token: %s\n", message.session_token);
    }
    else{
      printf("%s\n", cJSON_Print(message.data));
    }
    
    
    break;
  }
  
  case TICKET_CONSULTATION_MESSAGE_CODE:{
    if(cJSON_GetArraySize(message.data)==0){
      printf("No tickets linked to this user.\n");
      break;
    }

    cJSON *jsonTicket;
    cJSON_ArrayForEach(jsonTicket, message.data){
      Ticket ticket;
      parseJSONToTicket(jsonTicket, &ticket);
      printf("Ticket number: %d\n%s\n%s\nPriority: %s\nStatus: %s\nCreated on: %d/%d/%d\nAgent assigned: %s\n", 
        ticket.id, ticket.title, ticket.description, 
        ticket.priority==HIGH? "High":ticket.priority==MEDIUM? "Medium":"Low", 
        ticket.state==OPEN_STATE? "Open":ticket.state==IN_PROGRESS_STATE? "In progress":"Closed", 
        ticket.date.day, ticket.date.month, ticket.date.year, 
        ticket.state==OPEN_STATE? "No agent assigned":ticket.agent);
    }

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


  // char sessionToken[SESSION_TOKEN_LENGTH];
  


  

  


  
  
  
  
  
  
  
  
  
  
  User userData = {0};
  Message message = {0};
  


  //* SIGN UP
  // message.action_code = SIGNUP_MESSAGE_CODE;
  // User user;
  // strcpy(user.username, "nuovoUsername");
  // strcpy(user.password, "password");
  // message.data = parseUserToJSON(&user);


  //* AGENT CREATION

  message.action_code = CREATE_AGENT_MESSAGE_CODE;
  User user;
  strcpy(user.username, "agentUsername");
  strcpy(user.password, "agentPassword");
  strcpy(message.session_token, "D@Y7XB%G)XwUK]'O");
  message.data = parseUserToJSON(&user);







  
  





  //* writing to server
  char *stringMessage = cJSON_Print(parseMessageToJSON(&message));
  write(clientfd, stringMessage, strlen(stringMessage));
  write(clientfd, "\0", 1);
  printf("Message sent to server.\n");
  free(stringMessage);

  //* client receiving
  char receivedMessage[256]= {0};
  if(readMessage(clientfd, receivedMessage)==0){
    printf("The server has crashed. Try again in a while.\n");
    // break;
  }
  // printf("received %s\n", receivedMessage);
  handleMessage(receivedMessage, &userData);
  
  
  
  
  
  
  
  
  
  
  
  
  // int keepGoing = 1;
  // while (keepGoing)
  // {
  //   //! si potrebbe mettere un print diverso in base al ruolo dopo il login
  //   //! guardare se pulire console
  //   Message message = {0};
  //   int operation;

  //   printf("Select operation:\n"
  //     "1: Sign Up\n"
  //     "2: Sign In\n"
  //     "3: Create ticket\n"
  //     "4: View your tickets\n"
  //     "5: Release ticket\n"
  //     "0: Log out\n");
    
  //   scanf("%d", &operation);

  //   switch(operation){
  //     case 1:{
  //       //* SIGN UP
  //       message.action_code = SIGNUP_MESSAGE_CODE;
  //       User user;
  //       strcpy(user.username, "nuovoUsername");
  //       strcpy(user.password, "password");
  //       message.data = parseUserToJSON(&user);
  //       break;
  //     }
  //     case 2:{
  //       //* SIGN IN
  //       message.action_code = LOGIN_REQUEST_MESSAGE_CODE;
  //       User user;
  //       strcpy(user.username, "nuovoUsername");
  //       strcpy(user.password, "passwsord");
  //       message.data = parseUserToJSON(&user);
  //       break;
  //     }
  //     case 3:{
  //       //* CREAZIONE TICKET 
  //       Ticket ticket;
  //       strncpy(ticket.title, "Ticket Titolo", strlen("Ticket Titolo"));
  //       strncpy(ticket.description, "Ticket description", strlen("Ticket description"));
  //       ticket.date.day = 1;
  //       ticket.date.month = 1;
  //       ticket.date.year = 2023;
  //       ticket.priority = MEDIUM;
  //       ticket.state = OPEN_STATE;
  //       message.action_code = CREATE_TICKET_MESSAGE_CODE;
  //       message.data = parseTicketToJSON(&ticket);
  //       strcpy(message.session_token, "D@Y7XB%G)XwUK]'O");
  //       break;
  //     }
  //     case 4:{
  //       //* TICKET CONSULTATION
  //       message.action_code = TICKET_CONSULTATION_MESSAGE_CODE;
  //       strcpy(message.session_token, "D@Y7XB%G)XwUK]'O");
  //       break;
  //     }
  //     case 5:{
  //       //* RESOLVE TICKET
  //       message.action_code = RESOLVE_TICKET_MESSAGE_CODE;
  //       strcpy(message.session_token, "WXJ{)#vi3Lo(Hb5{");
  //       Ticket ticket = {0};
  //       ticket.id = 2;
  //       message.data = parseTicketToJSON(&ticket);
  //       break;
  //     }
  //     case 0:{
  //       //* EXIT
  //       keepGoing = 0;
  //       message.action_code = CLOSE_CONNECTION_MESSAGE_CODE;
  //       message.data = cJSON_CreateString("");
  //       strcpy(message.session_token, "WXJ{)#vi3Lo(Hb5{");
  //       break;
  //     }

  //     default:{
  //       printf("Invalid operation.\n");
  //       continue;
  //     }
  //   }

  //   //* writing to server
  //   char *stringMessage = cJSON_Print(parseMessageToJSON(&message));
  //   write(clientfd, stringMessage, strlen(stringMessage));
  //   write(clientfd, "\0", 1);
  //   printf("Message sent to server.\n");
  //   free(stringMessage);

  //   //* client receiving
  //   char receivedMessage[256]= {0};
  //   if(readMessage(clientfd, receivedMessage)==0){
  //     printf("The server has crashed. Try again in a while.\n");
  //     break;
  //   }
  //   // printf("received %s\n", receivedMessage);
  //   handleMessage(receivedMessage, &userData);
  // }
  



  













  //! TESTING MULTIPLE CONNECTIONS, DELETE LATER
  // sleep(10);


  // close socket
  close(clientfd);
  printf("Connection closed.\n");


  exit(0);
}