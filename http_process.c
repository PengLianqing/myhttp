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
#include "http_process.h"
#include "file_process.h"

/**
  * @brief          http响应
  * 1）解析请求路径，并根据目录/文件作出响应响应
  * 2）发送http头和文件/目录
  * @param[in]      bev,path
  * @retval         0
  */
int response_http(struct bufferevent *bev, const char *method, char *path)
{
    if(strcasecmp("GET", method) == 0){
        //对路径进行解码%21 '!' %22 '"' %23 '#' %24 '$'
        strdecode(path, path);
        char *pf = &path[1];

        //把/转化为./
        //否则http://211.67.19.181:6202/直接跳转到404
        if(strcmp(path, "/") == 0 || strcmp(path, "/.") == 0)
        {
            pf="./";
        }
        printf("***** http Request Resource Path =  %s, pf = %s\n", path, pf);

        //判断文件/文件夹是否存在
        struct stat sb;
        if(stat(pf,&sb) < 0)
        {
            perror("open file err:");
            send_error(bev, 404, "Not Found", "NO such file or direntry"); 
            return -1;
        }
        if(S_ISDIR(sb.st_mode))//处理目录
        {
            //应该显示目录列表
            send_response(bev, 200, "OK", (char *)get_file_type(".html"), -1);
            send_dir(bev, pf);
        }
        else //处理文件
        {
            send_response(bev, 200, "OK", (char *)get_file_type(pf), sb.st_size);
            send_file_to_http(bev ,pf);
        }
    }
    return 0;
}

/**
  * @brief          解析http请求消息的每一行内容，程序中没有使用
  * @param[in]      NULL
  * @retval         NULL
  */
int get_line(int sock, char *buf, int size)
{
    int i = 0;
    char c = '\0';
    int n;
    while ((i < size - 1) && (c != '\n')) {    
        n = recv(sock, &c, 1, 0);
        if (n > 0) {        
            if (c == '\r') {            
                n = recv(sock, &c, 1, MSG_PEEK); //拷贝读
                if ((n > 0) && (c == '\n')) {               
                    recv(sock, &c, 1, 0);
                } else {                            
                    c = '\n';
                }
            }
            buf[i] = c;
            i++;
        } else {       
            c = '\n';
        }
    }
    buf[i] = '\0';
    return i;
}

/**
  * @brief          发送HTTP应答
  * @param[in]      客户端fd,错误号,错误描述,回发文件类型,文件长度
  *                 (bev, 200, "OK", (char *)get_file_type(".html"), -1);
  * @retval         NULL
  */
/******************************
 * http应答内容：
	1. Http1.1 200 OK
	2. Server: xhttpd
	3. Content-Type：text/plain; charset=iso-8859-1 (内容类型）
	4. Date: Fri, 18 Jul 2014 14:34:26 GMT
	5. Content-Length: 32  （要么不写或者传-1，要写务必精确）
	6. Content-Language: zh-CN
	7. Last-Modified: Fri, 18 Jul 2014 08:36:36 GMT
	8. Connection: close （发送后连接断开）
	9. \r\n
    10. 文件内容……
*/
#define _HTTP_CLOSE_ "Connection: close\r\n"
void send_response(struct bufferevent *bev ,int no ,char* desp ,char *type ,long len)
{
    char buf[256]={0};

    sprintf(buf, "HTTP/1.1 %d %s\r\n", no, desp);
    //HTTP/1.1 200 OK\r\n
    bufferevent_write(bev, buf, strlen(buf));
    // 文件类型
    sprintf(buf, "Content-Type:%s\r\n", type);
    bufferevent_write(bev, buf, strlen(buf));
    // 文件大小
    sprintf(buf, "Content-Length:%ld\r\n", len);
    bufferevent_write(bev, buf, strlen(buf));
    // Connection: close
    bufferevent_write(bev, _HTTP_CLOSE_, strlen(_HTTP_CLOSE_));
    //send \r\n
    bufferevent_write(bev, "\r\n", 2);
    return;
}

/**
  * @brief          发送错误404页面
  * @param[in]      send_error(bev, 404, "Not Found", "NO such file or direntry"); 
  * @retval         NULL
  */
void send_error(struct bufferevent *bev, int status, char *title, char *text)
{
   char buf[4096] = {0};

	sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
	sprintf(buf+strlen(buf), "Content-Type:%s\r\n", "text/html");
	sprintf(buf+strlen(buf), "Content-Length:%d\r\n", -1);
	sprintf(buf+strlen(buf), "Connection: close\r\n");
    bufferevent_write(bev, buf, strlen(buf));
    bufferevent_write(bev, "\r\n", 2);
	memset(buf, 0, sizeof(buf)); //清空buf

	sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
	sprintf(buf+strlen(buf), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
	sprintf(buf+strlen(buf), "%s\n", text);
	sprintf(buf+strlen(buf), "<hr>\n</body>\n</html>\n");
    bufferevent_write(bev, buf, strlen(buf));
	return ; 
}

/**
  * @brief          16进制数转化为10进制
  * @param[in]      hex 
  * @retval         int
  */
int hexit(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    return 0;
}

/**
  * @brief          编码，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
  * @param[in]      strencode(encoded_name, sizeof(encoded_name), name); 
  * @retval         int
  */
void strencode(char* to, size_t tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from)
    {
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0)
        {
            *to = *from;
            ++to;
            ++tolen;
        }
        else
        {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

/**
  * @brief          解码，从浏览器读，将URL编码转为文件名。
  * @param[in]      strdecode(path, path);
  * @retval         NULL
  */
/*
 * 这里的内容是处理%20之类的东西！是"解码"过程。
 * %20 URL编码中的‘ ’(space)
 * %21 '!' %22 '"' %23 '#' %24 '$'
 * %25 '%' %26 '&' %27 ''' %28 '('......
 * 相关知识html中的‘ ’(space)是&nbsp
 */
void strdecode(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from)
    {
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2]))
        {
            // 依次判断from中 %20 三个字符
            *to = hexit(from[1])*16 + hexit(from[2]);
            // 移过已经处理的两个字符(%21指针指向1),表达式3的++from还会再向后移一个字符
            from += 2;
        }
        else
        {
            *to = *from;
        }
    }
    *to = '\0';
}