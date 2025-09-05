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

// asks user for ticket info
void createTicket(Ticket *ticket) {
  printf("Enter the ticket details:\n");
  printf("Title:\n");
  fgets(ticket->title, sizeof(ticket->title), stdin); 
  ticket->title[strcspn(ticket->title, "\n")] = 0; // remove newline character

  printf("Description:\n");
  fgets(ticket->description, sizeof(ticket->description), stdin);
  ticket->description[strcspn(ticket->description, "\n")] = 0; // remove newline character

  while(1) {
    printf("Priority (1: Low, 2: Medium, 3: High):\n");
    scanf("%d", &ticket->priority);
    while(getchar() != '\n'); // clear input buffer
    if (ticket->priority < MIN_PRIORITY || ticket->priority > MAX_PRIORITY) {
      printf("Invalid priority, try again.\n");
      continue;
    }
    break;
  }
}

// asks user for input credentials
void getUserCredentials(User *user){
  printf("Enter username: ");
  fgets(user->username, sizeof(user->username), stdin);
  user->username[strcspn(user->username, "\n")] = 0; // remove newline character
  user->username[sizeof(user->username)-1] = '\0'; // in case username was too long

  printf("Enter password: ");
  fgets(user->password, sizeof(user->password), stdin);
  user->password[strcspn(user->password, "\n")] = 0; // remove newline character
  user->password[sizeof(user->password)-1] = '\0'; // in case password was too long
}

// asks user for id of ticket to resolve
void getTicketIdToResolve(Ticket *ticket){
  printf("Enter ticket id to resolve: ");
  scanf("%d", &ticket->id); // !! se vogliamo usare fgets poi dobbiamo convertire in int usando atoi, che facciamo ??
  while(getchar() != '\n'); // clear input buffer
}

// asks admin the new ticket priority 
void getTicketPriorityToUpdate(Ticket *ticket){
  printf("Enter ticket id:\n");
  scanf("%d", &ticket->id);
  while(getchar() != '\n'); // clear input buffer
  while(1) {
    printf("Enter new ticket priority (1: Low, 2: Medium, 3: High):\n");
    scanf("%d", &ticket->priority); 
    while(getchar() != '\n'); // clear input buffer
    if (ticket->priority < MIN_PRIORITY || ticket->priority > MAX_PRIORITY) {
      printf("Invalid priority, try again.\n");
      continue;
    }
    break;
  }
}

// asks admin the new ticket agent
void getTicketAgentToUpdate(Ticket *ticket){
  printf("Enter ticket id: ");
  scanf("%d", &ticket->id); 
  while(getchar() != '\n'); // clear input buffer

  printf("Enter username: ");
  fgets(ticket->agent, sizeof(ticket->agent), stdin);
  ticket->agent[strcspn(ticket->agent, "\n")] = 0; // remove newline character
}


// message interpretation
void handleMessage(char *stringMessage, User *userData){
  Message message = {0};
  if(parseJSONToMessage(cJSON_Parse(stringMessage), &message) == GENERAL_ERROR_CODE){
    printf("Server sent an invalid response\n");
    return;
  }

  switch (message.action_code)
  {
    // outputs on console whatever the server sent
    case INFO_MESSAGE_CODE:{
      printf("Server: %s\n", cJSON_Print(message.data));
      break;
    }
    // saves token into userData for future requests
    case LOGIN_INFO_MESSAGE_CODE:{
      if(strlen(message.session_token)!=0){
        strcpy(userData->token, message.session_token);
      }
      printf("%s\n", cJSON_Print(message.data));
      break;
    }
    // prints ticket list
    case TICKET_CONSULTATION_MESSAGE_CODE:{
      if(cJSON_GetArraySize(message.data)==0){
        printf("No tickets linked to this user.\n");
        break;
      }

      cJSON *jsonTicket;
      cJSON_ArrayForEach(jsonTicket, message.data){
        Ticket ticket = {0};
        parseJSONToTicket(jsonTicket, &ticket);
        printf("Ticket number: %d\n%s\n%s\nPriority: %s\nStatus: %s\nCreated on: %d/%d/%d\nAgent assigned: %s\n\n", 
          ticket.id, ticket.title, ticket.description, 
          ticket.priority==HIGH? "High":ticket.priority==MEDIUM? "Medium":"Low", 
          ticket.state==OPEN_STATE? "Open":ticket.state==IN_PROGRESS_STATE? "In progress":"Closed", 
          ticket.date.day, ticket.date.month, ticket.date.year, 
          ticket.state==OPEN_STATE? "No agent assigned":ticket.agent);
      }

      break;
    }
    // closes the connection
    case CLOSE_CONNECTION_MESSAGE_CODE:{
      printf("Server has closed the connection.\n");
      return; // skips "press any key to continue"
    }

    default:{
      printf("ERROR: Invalid action_code.\n");
      break;
    }
  }
  printf("Press ENTER to continue.\n");
  getchar();
}

int main(int argc, char *argv[]){
  printf("Client started.\n");
  int clientfd, resultCode;
  struct sockaddr_in serverAddress;
  
  // sets server address
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(SERVER_PORT);
  serverAddress.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);

  // connection
  do
  {
    // creates socket in while do wait server if it's down
    clientfd = socket(AF_INET, SOCK_STREAM, 0);
    resultCode = connect(clientfd, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    if(resultCode == -1) {
      perror("Error connecting to server");
      sleep(1);
    }
  } while (resultCode == -1);

  printf("Connected to server at %s:%d\n", SERVER_ADDRESS, SERVER_PORT);

  
  User userData = {0};
  int keepGoing = 1;
  while (keepGoing)
  {
    Message message = {0};
    int operation;

    system("clear");
    printf("Select operation:\n"
      "1: Sign Up\n"
      "2: Sign In\n"
      "3: Create ticket\n"
      "4: View your tickets\n"
      "5: Release ticket\n"
      "6: Update ticket priority\n"
      "7: Assign new agent to ticket\n"
      "8: Create agent account\n"
      "0: Log out\n");
    
    scanf("%d", &operation);
    char c;
    while ((c = getchar()) != '\n' && c != EOF); // empties buffer to get rid of \n
    system("clear");

    // creates Message according to selected operation
    switch(operation){
      case 1:{
        //* SIGN UP
        message.action_code = SIGNUP_MESSAGE_CODE;
        User user;
        getUserCredentials(&user);
        message.data = parseUserToJSON(&user);
        break;
      }
      case 2:{
        //* SIGN IN
        message.action_code = LOGIN_REQUEST_MESSAGE_CODE;
        User user;
        getUserCredentials(&user);
        message.data = parseUserToJSON(&user);
        break;
      }
      case 3:{
        //* CREAZIONE TICKET 
        Ticket ticket = {0};
        createTicket(&ticket);
        message.action_code = CREATE_TICKET_MESSAGE_CODE;
        message.data = parseTicketToJSON(&ticket);
        strcpy(message.session_token, userData.token);
        break;
      }
      case 4:{
        //* TICKET CONSULTATION
        message.action_code = TICKET_CONSULTATION_MESSAGE_CODE;
        strcpy(message.session_token, userData.token);
        break;
      }
      case 5:{
        //* RESOLVE TICKET
        Ticket ticket = {0};
        getTicketIdToResolve(&ticket);
        message.action_code = RESOLVE_TICKET_MESSAGE_CODE;
        message.data = parseTicketToJSON(&ticket);
        strcpy(message.session_token, userData.token);
        break;
      }
      case 6:{
        //* UPDATE TICKET STATE AS ADMIN
        message.action_code = UPDATE_TICKET_PRIORITY_MESSAGE_CODE;
        Ticket ticket = {0};
        getTicketPriorityToUpdate(&ticket);
        message.data = parseTicketToJSON(&ticket);
        strcpy(message.session_token, userData.token);
        break;
      }
      case 7:{
        //* ASSIGN AGENT TO TICKET AS ADMIN
        Ticket ticket = {0};
        getTicketAgentToUpdate(&ticket);
        message.action_code = ASSIGN_AGENT_MESSAGE_CODE;
        message.data = parseTicketToJSON(&ticket);
        strcpy(message.session_token, userData.token);
        break;
      }
      case 8:{
        //* AGENT CREATION AS ADMIN
        User user = {0};
        getUserCredentials(&user);
        message.action_code = CREATE_AGENT_MESSAGE_CODE;
        message.data = parseUserToJSON(&user);
        strcpy(message.session_token, userData.token);
        break;
      }
      case 0:{
        //* EXIT
        keepGoing = 0;
        message.action_code = CLOSE_CONNECTION_MESSAGE_CODE;
        message.data = cJSON_CreateString("");
        strcpy(message.session_token, userData.token);
        break;
      }

      default:{
        printf("Invalid operation.\n");
        continue;
      }
    }

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
      break;
    }
    // printf("received %s\n", receivedMessage);
    handleMessage(receivedMessage, &userData);
  }

  // close socket
  close(clientfd);
  printf("Connection closed.\n");
  exit(0);
}