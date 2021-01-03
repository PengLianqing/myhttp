/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       http_process.c/h
  * @brief      
  * @note       
  * @history
  *  Version    Date            Author          Modification
  *  V1.0.0     Jan-1-2021      Peng            1. 完成
  *
  @verbatim
  ==============================================================================

  ==============================================================================
  @endverbatim
  ****************************(C) COPYRIGHT 2021 Peng****************************
*/
#pragma once

#include "main.h"

//stat
#include <sys/types.h>
#include <sys/stat.h>
//type
#include <ctype.h>

void http_read(struct bufferevent *bev);

int response_http(struct bufferevent *bev, const char *method, char *path);

void send_response(struct bufferevent *bev ,int no ,char* disp ,char *type ,long len);
void http_request(struct bufferevent *bev ,char *path);

void send_error(struct bufferevent *bev, int status, char *title, char *text);

void strencode(char* to, size_t tosize, const char* from);
void decode_str(char *to, char *from);
void encode_str(char* to, int tosize, const char* from);
int hexit(char c);