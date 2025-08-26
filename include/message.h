#ifndef JSONMESSAGE_H
#define JSONMESSAGE_H

#define SESSION_TOKEN_LENGTH 16
#define INITIAL_MESSAGE_SIZE 16
#define CREATE_TICKET_MESSAGE_CODE 0
#define INFO_MESSAGE_CODE 1
#define LOGIN_REQUEST_MESSAGE_CODE 2
#define LOGIN_INFO_MESSAGE_CODE 3
#define SIGNIN_MESSAGE_CODE 4

#define MAX_USERNAME_LENGTH 20
#define MAX_PASSWORD_LENGTH 20

#include "../lib/cJSON/cJSON.h"

typedef struct struct_json_message
{
  int action_code;
  cJSON *data;
  char session_token[SESSION_TOKEN_LENGTH+1];
} Message;

typedef enum login_request_type{
  LOGIN_REQUEST = 0,
  LOGIN_SUCCESSFUL_RESPONSE = 1,
  SIGNIN_REQUEST = 2,
} LoginRequestType;

typedef struct login_data
{
  LoginRequestType request_type;
  char username[MAX_USERNAME_LENGTH];
  char password[MAX_PASSWORD_LENGTH];
  char token[SESSION_TOKEN_LENGTH+1];
} LoginData;


#endif