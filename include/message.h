#ifndef MESSAGE_H
#define MESSAGE_H

#define SESSION_TOKEN_LENGTH 16
#define INITIAL_MESSAGE_SIZE 16
#define CREATE_TICKET_MESSAGE_CODE 0
#define INFO_MESSAGE_CODE 1
#define LOGIN_REQUEST_MESSAGE_CODE 2
#define LOGIN_INFO_MESSAGE_CODE 3
#define SIGNIN_MESSAGE_CODE 4
#define TICKET_CONSULTATION_MESSAGE_CODE 5
#define RESOLVE_TICKET_CODE 6
#define UPDATE_TICKET_CODE 7

#define GENERAL_ERROR_CODE -1

#define MAX_USERNAME_LENGTH 20
#define MAX_PASSWORD_LENGTH 20

#include "../lib/cJSON/cJSON.h"

typedef struct struct_json_message{
  int action_code;
  cJSON *data;
  char session_token[SESSION_TOKEN_LENGTH+1];
} Message;

typedef enum login_request_type{
  LOGIN_REQUEST = 0,
  LOGIN_SUCCESSFUL_RESPONSE = 1,
  SIGNIN_REQUEST = 2,
} LoginRequestType;

typedef enum role{
  USER_ROLE = 0,
  AGENT_ROLE = 1,
  ADMIN_ROLE = 2
} Role;

typedef struct login_data{
  LoginRequestType request_type;
  Role role;
  char username[MAX_USERNAME_LENGTH+1];
  char password[MAX_PASSWORD_LENGTH+1];
  char token[SESSION_TOKEN_LENGTH+1];
} LoginData;

typedef struct user{
  char username[MAX_USERNAME_LENGTH+1];
  
} User;


#endif