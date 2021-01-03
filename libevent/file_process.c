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
#include "file_process.h"
#include "http_process.h"

int send_file_to_http(struct bufferevent *bev ,const char *filename)
{
    //打开文件
    int fd = open(filename ,O_RDONLY);
    if(fd == -1)
    {
        perror("open error");
        exit(1);
    }

    //循环读文件并传输数据
    char buf[4096] = {0};
    int len=0 ;
    while( (len = read(fd ,buf ,sizeof(buf))) > 0 ) {

        //printf("read %ld ,%s\n",len ,buf);
        bufferevent_write(bev, buf, len);
        memset(buf ,0 ,len);
        printf("send %s ,%d bytes.\n",filename ,len);
    }
    if(len == -1){
        perror("read error");
        exit(1);
    }
    close(fd);
    return 0;
}

/*
     *charset=iso-8859-1	西欧的编码，说明网站采用的编码是英文；
     *charset=gb2312		说明网站采用的编码是简体中文；
     *charset=utf-8			代表世界通用的语言编码；
     *						可以用到中文、韩文、日文等世界上所有语言编码上
     *charset=euc-kr		说明网站采用的编码是韩文；
     *charset=big5			说明网站采用的编码是繁体中文；
     *
     *以下是依据传递进来的文件名，使用后缀判断是何种文件类型
     *将对应的文件类型按照http定义的关键字发送回去
*/
const char *get_file_type(char *name)
{
    char* dot;

    dot = strrchr(name, '.');	//自右向左查找‘.’字符;如不存在返回NULL

    if (dot == (char*)0)
        return "text/plain; charset=utf-8";
    if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
        return "text/html; charset=utf-8";
    if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
        return "image/jpeg";
    if (strcmp(dot, ".gif") == 0)
        return "image/gif";
    if (strcmp(dot, ".png") == 0)
        return "image/png";
    if (strcmp(dot, ".css") == 0)
        return "text/css";
    if (strcmp(dot, ".au") == 0)
        return "audio/basic";
    if (strcmp( dot, ".wav") == 0)
        return "audio/wav";
    if (strcmp(dot, ".avi") == 0)
        return "video/x-msvideo";
    if (strcmp(dot, ".mov") == 0 || strcmp(dot, ".qt") == 0)
        return "video/quicktime";
    if (strcmp(dot, ".mpeg") == 0 || strcmp(dot, ".mpe") == 0 || strcmp(dot, ".mp4") == 0)
        return "video/mpeg";
    if (strcmp(dot, ".vrml") == 0 || strcmp(dot, ".wrl") == 0)
        return "model/vrml";
    if (strcmp(dot, ".midi") == 0 || strcmp(dot, ".mid") == 0)
        return "audio/midi";
    if (strcmp(dot, ".mp3") == 0)
        return "audio/mpeg";
    if (strcmp(dot, ".ogg") == 0)
        return "application/ogg";
    if (strcmp(dot, ".pac") == 0)
        return "application/x-ns-proxy-autoconfig";

    return "text/plain; charset=utf-8";
}

//扫描目录并以html格式发送目录
int send_dir(struct bufferevent *bev,const char *dirname)
{
    char encoded_name[1024];
    char path[1024];
    char timestr[64];
    struct stat sb;
    struct dirent **dirinfo;
    int i;

    char buf[4096] = {0};
    sprintf(buf, "<html><head><meta charset=\"utf-8\"><title>%s</title></head>", dirname);
    sprintf(buf+strlen(buf), "<body><h1>当前目录：%s</h1><table>", dirname);
    //添加目录内容
    int num = scandir(dirname, &dirinfo, NULL, alphasort);
    for(i=0; i<num; ++i)
    {
        // 编码
        strencode(encoded_name, sizeof(encoded_name), dirinfo[i]->d_name);

        sprintf(path, "%s%s", dirname, dirinfo[i]->d_name);
        printf("############# path = %s\n", path);
        if (lstat(path, &sb) < 0)
        {
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s\">%s</a></td></tr>\n", 
                    encoded_name, dirinfo[i]->d_name);
        }
        else
        {
            strftime(timestr, sizeof(timestr), 
                     "  %d  %b   %Y  %H:%M", localtime(&sb.st_mtime));
            if(S_ISDIR(sb.st_mode))
            {
                sprintf(buf+strlen(buf), 
                        "<tr><td><a href=\"%s/\">%s/</a></td><td>%s</td><td>%ld</td></tr>\n",
                        encoded_name, dirinfo[i]->d_name, timestr, sb.st_size);
            }
            else
            {
                sprintf(buf+strlen(buf), 
                        "<tr><td><a href=\"%s\">%s</a></td><td>%s</td><td>%ld</td></tr>\n", 
                        encoded_name, dirinfo[i]->d_name, timestr, sb.st_size);
            }
        }
        bufferevent_write(bev, buf, strlen(buf));
        memset(buf, 0, sizeof(buf));
    }
    sprintf(buf+strlen(buf), "</table></body></html>");
    bufferevent_write(bev, buf, strlen(buf));
    printf("################# Dir Read OK !!!!!!!!!!!!!!\n");

    return 0;
}