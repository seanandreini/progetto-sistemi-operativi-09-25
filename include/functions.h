#define CREATE_TICKET_CODE 0
#define MESSAGE_CODE 1

int readMessage(int fd, char *string);
void handleMessage(char *message);