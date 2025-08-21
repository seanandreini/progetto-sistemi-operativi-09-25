#include <unistd.h>

// read until null terminator
int readLine(int fd, char *string) {
  int bytesRead;

  do{
    bytesRead = read(fd, string, 1);
  }
  while(bytesRead>0 && *string++ != '\0');

  return bytesRead;
}