#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

#define SESSION_TOKEN_LENGTH 16
#define INITIAL_MESSAGE_SIZE 16
#define CREATE_TICKET_CODE 0
#define MESSAGE_CODE 1
#define LOGIN_REQUEST_CODE 2
#define LOGIN_INFO_CODE 3

#define MAX_USERNAME_LENGTH 20
#define MAX_PASSWORD_LENGTH 20

#include "../lib/cJSON/cJSON.h"

typedef struct struct_json_message
{
  int action_code;
  cJSON *data;
} Message;

typedef enum login_request_type{
  LOGIN_REQUEST = 0,
  LOGIN_SUCCESSFUL_RESPONSE = 1,
  LOGIN_FAILED_RESPONSE = 2
} LoginRequestType;

typedef struct login_data
{
  LoginRequestType request_type;
  char username[MAX_USERNAME_LENGTH];
  char password[MAX_PASSWORD_LENGTH];
  char token[SESSION_TOKEN_LENGTH+1];
} LoginData;


#endif