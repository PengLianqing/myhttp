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

int send_file_to_http(struct bufferevent *bev ,const char *filename);

const char *get_file_type(char *name);

int send_dir(struct bufferevent *bev,const char *dirname);

