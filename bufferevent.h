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

/**
  * @brief          监听事件的回调函数
  * 1）输出连接客户端的地址结构；
  * 2）创建bufferevent事件，对应read_cb和event_cb回调函数。
  * @param[in]      内核自动传入
  * @retval         NULL
  */
void listener_cb(
                 struct evconnlistener *listener ,
                 evutil_socket_t fd ,
                 struct sockaddr *addr ,
                 int len ,void *ptr);

/**
  * @brief          bufferevent的读回调函数
  * 1）读取http请求；
  * 2）做出http响应response_http()。
  * @param[in]      内核自动传入,bev
  * @retval         NULL
  */
void read_cb(struct bufferevent *bev ,void *arg);

/**
  * @brief          bufferevent的事件回调函数
  * event响应，如果是BEV_EVENT_CONNECTED则返回，发生错误或连接断开则释放资源。
  * @param[in]      内核自动传入,bev,events
  * @retval         NULL
  */
void event_cb(struct bufferevent *bev , short events ,void *arg);

/**
  * @brief          信号事件回调函数
  * 接收到信号如ctrl+c，此时延时1s后退出程序。
  * @param[in]      内核自动传入,bev,events
  * @retval         NULL
  */
void signal_cb(evutil_socket_t sig, short events, void *user_data);