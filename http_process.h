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

/**
  * @brief          http响应
  * 1）解析请求路径，并根据目录/文件作出响应响应
  * 2）发送http头和文件/目录
  * @param[in]      bev,path
  * @retval         0
  */
int response_http(struct bufferevent *bev, const char *method, char *path);

/**
  * @brief          解析http请求消息的每一行内容，程序中没有使用
  * @param[in]      NULL
  * @retval         NULL
  */
int get_line(int sock, char *buf, int size);

/**
  * @brief          发送HTTP应答
  * @param[in]      客户端fd,错误号,错误描述,回发文件类型,文件长度
  *                 (bev, 200, "OK", (char *)get_file_type(".html"), -1);
  * @retval         NULL
  */
void send_response(struct bufferevent *bev ,int no ,char* desp ,char *type ,long len);

/**
  * @brief          发送错误404页面
  * @param[in]      send_error(bev, 404, "Not Found", "NO such file or direntry"); 
  * @retval         NULL
  */
void send_error(struct bufferevent *bev, int status, char *title, char *text);

/**
  * @brief          16进制数转化为10进制
  * @param[in]      hex 
  * @retval         int
  */
int hexit(char c);

/**
  * @brief          编码，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
  * @param[in]      strencode(encoded_name, sizeof(encoded_name), name); 
  * @retval         int
  */
void strencode(char* to, size_t tosize, const char* from);

/**
  * @brief          解码，从浏览器读，将URL编码转为文件名。
  * @param[in]      strdecode(path, path);
  * @retval         NULL
  */
void strdecode(char *to, char *from);