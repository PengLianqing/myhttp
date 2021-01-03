/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       bufferevent.c/h
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

void listener_cb(
                 struct evconnlistener *listener ,
                 evutil_socket_t fd ,
                 struct sockaddr *addr ,
                 int len ,void *ptr);

void read_cb(struct bufferevent *bev ,void *arg);
void write_cb(struct bufferevent *bev ,void *arg);
void event_cb(struct bufferevent *bev , short events ,void *arg);

void signal_cb(evutil_socket_t sig, short events, void *user_data);
