/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       file_process.c/h
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
//fcntl
#include <unistd.h>
#include <fcntl.h>
//scandir
#include <dirent.h>

/**
  * @brief          http发送文件
  * 循环读文件并传输数据
  * @param[in]      bev,filename
  * @retval         0
  */
int send_file_to_http(struct bufferevent *bev ,const char *filename);

/**
  * @brief          获取文件类型，并返回http关键字
  * @param[in]      name
  * @retval         http关键字
  */
const char *get_file_type(char *name);

/**
  * @brief          扫描目录并以html格式发送目录
  * @param[in]      bev,dirname
  * @retval         0
  */
int send_dir(struct bufferevent *bev,const char *dirname);
