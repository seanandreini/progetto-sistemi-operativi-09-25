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
#include "ticket.h"

#define SERVER_PORT 12345
#define SERVER_ADDRESS "127.0.0.1"


//TODO: struttura per ticket

// read until null terminator
int readLine(int fd, char *string) {
  int bytesRead;

  do{
    bytesRead = read(fd, string, 1);
  }
  while(bytesRead>0 && *string++ != '\0');

  return bytesRead;
}
/*
void pulisci_buffer_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}*/

ticket creaTicket() {
  ticket t;
  printf("Inserisci i dati del ticket:\n");
  printf("Titolo:\n");
  fgets(t.title, sizeof(t.title), stdin); 
  printf("descrizione:\n");
  fgets(t.description, sizeof(t.description), stdin);
  printf("Data (gg/mm/aaaa):\n");
  scanf("%d/%d/%d", &t.date.giorno, &t.date.mese, &t.date.anno);
  int temp_priority;
  printf("Priorità (1: Bassa, 2: Media, 3: Alta):\n");
  scanf("%d", &temp_priority);
  t.priority = temp_priority; 
  int temp_state;
  printf("Stato (1: Aperto, 2: In Corso, 3: Chiuso):\n");
  scanf("%d", &temp_state);
  t.state = temp_state;
  //Dati agente non inseriti da tastiera, client non deve scegliere l'agente
  return t;
  //alcune cose forse dovrebbe inserirle il server, oltre all'id del ticket
  //gemini dice di pulire il buffer dopo scanf per rimuovere l'\n rimasto che altrimenti fgets si ferma subito. 
  //controllare anche inout enum, ho inserito 4 e non h adato errore, come ci si comporta in questo caso?
}

int main(int argc, char *argv[]){
  printf("Client started.\n");
  int clientfd, resultCode;
  struct sockaddr_in serverAddress;
  printf("Creo ticket asoe");
  ticket t = creaTicket();
  printf("Ticket creato: %d, %s, %s\n", t.id, t.title, t.description);
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

  //TODO: scrittura ticket (IN JSON)
  char *message = "Il Bertini è in vacanza\n";
  write(clientfd, message, strlen(message)+1); // +1 to include \0
  printf("Sent to server: Hello from client!\n");  

  // close socket
  close(clientfd);
  printf("Connection closed.\n");


  exit(0);
}