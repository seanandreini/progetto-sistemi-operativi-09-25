#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"


int main(){
  // FILE *file = fopen("./ticketList.json", "w");

  // cJSON *jsonList = cJSON_CreateArray();

  // cJSON *jsonObject1 = cJSON_CreateObject();
  // cJSON_AddStringToObject(jsonObject1, "name", "Ticket 1");
  // cJSON_AddNumberToObject(jsonObject1, "id", 1);
  // cJSON_AddItemToArray(jsonList, jsonObject1);

  // cJSON *jsonObject2 = cJSON_CreateObject();
  // cJSON_AddStringToObject(jsonObject2, "name", "Ticket 2");
  // cJSON_AddNumberToObject(jsonObject2, "id", 2);
  // cJSON_AddItemToArray(jsonList, jsonObject2);

  // char *jsonString = cJSON_Print(jsonList);
  // fprintf(file, "%s", jsonString);

  FILE *file = fopen("./ticketList.json", "r");

  fseek(file, 0, SEEK_END);
  long fileSize = ftell(file);
  fseek(file, 0, SEEK_SET);

  char *jsonString=malloc(fileSize+1);
  fread(jsonString, 1, fileSize, file);


  cJSON *jsonList = cJSON_Parse(jsonString);

  
  cJSON *item1 = cJSON_GetArrayItem(jsonList, 0);
  cJSON *item2 = cJSON_GetArrayItem(jsonList, 1);
  
  printf("%s\n", cJSON_GetObjectItem(item1, "name")->valuestring);
}