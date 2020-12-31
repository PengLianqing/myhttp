#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//fcntl
#include <unistd.h>
#include <fcntl.h>
//stat
#include <sys/types.h>
#include <sys/stat.h>
//socket
#include <sys/types.h>
#include <sys/socket.h>
#include "wrap.h"
//epoll
#include <sys/epoll.h>
//scandir
#include <dirent.h>
//isxdigit
#include <ctype.h>
//time
#include <time.h>

//user function
void http_read(int cfd ,int epfd);
int get_line(int sock, char *buf, int size);
void close_connection(int cfd ,int epfd);
void send_response(int cfd ,int no ,char* disp ,char *type ,int len);
void http_request(int cfd ,char *path);
void send_file(int cfd ,const char *file);
const char *get_file_type(const char *filename);
void send_error(int cfd, int status, char *title, char *text);
void send_dir(int cfd, const char* dirname);
void decode_str(char *to, char *from);
void encode_str(char* to, int tosize, const char* from);
int hexit(char c);


//user var
char buf[4096],write_buf[128];
char clin_ip[32];
int ret = -1;
int main()
{
    int lfd = 0,cfd = 0;
    char clin_ip[32];
            
    pid_t pid;

    //配置服务器地址结构
    struct sockaddr_in serv_addr , clin_addr;
    //memset(&serv_addr ,0 ,sizeof(serv_addr));
    bzero(&serv_addr ,sizeof(serv_addr)); //将地址结构清零 <strings.h>
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(6202); //本地套接字转网络短整型
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY); //本地套接字转长整型
  
    //创建socket文件描述符
    lfd = Socket(AF_INET,SOCK_STREAM,0);
    // 端口复用
    int flag = 1;
    setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    //为socket文件描述符绑定服务器端地址结构
    ret = Bind(lfd ,(struct sockaddr *)&serv_addr ,sizeof(serv_addr));
    //设置与服务器建立的最大连接数
    Listen(lfd ,128);

    //创建监听红黑树并将lfd挂到红黑树上
    int epfd = epoll_create(1024);
    if(epfd == -1)
    {
        perror("epoll creat error");
        exit(1);
    }
    struct epoll_event tep ,ep[1024];
    tep.events = EPOLLIN;
    tep.data.fd = lfd;
    ret = epoll_ctl(epfd ,EPOLL_CTL_ADD ,lfd ,&tep);
    if(ret==-1)
    {
        perror("epoll_ctr add lfd error");
        exit(1);
    }

    while(1)
    {
        //监听
        ret = epoll_wait(epfd ,ep ,1024 ,-1);
        if(ret==-1)
        {
            perror("epoll_wait error");
            exit(1);
        }

        for(int i=0 ;i<ret ;i++)
        {
            //只处理读事件, 其他事件默认不处理
            struct epoll_event *pev = &ep[i];
            if(!(pev->events & EPOLLIN)) {
                // 不是读事件
                continue;
            }
            if(pev->data.fd==lfd)
            {
                //新客户端建立连接
                socklen_t clin_addr_len = sizeof(clin_addr);
                cfd = Accept(lfd ,(struct sockaddr *)&clin_addr ,&clin_addr_len);

                //打印客户端地址结构
                printf("new connection: client ip %s ,port: %d\n",
                    inet_ntop(AF_INET,&(clin_addr.sin_addr.s_addr),clin_ip,sizeof(clin_ip)),
                    ntohs(clin_addr.sin_port));
                
                //设置cfd为非阻塞
                int flag = fcntl(cfd ,F_GETFL);
                flag |= O_NONBLOCK;
                fcntl(cfd ,F_SETFL ,flag);

                //将cfd挂到红黑树上
                //设置边沿非阻塞模式，否则会循环按行读取http请求
                tep.events = EPOLLIN | EPOLLET;
                tep.data.fd = cfd;
                epoll_ctl(epfd ,EPOLL_CTL_ADD ,cfd ,&tep);
            }
            else
            {
                http_read(pev->data.fd ,epfd);
            }
        }
    }
    printf("all done.\n");
    return 0;
}

void close_connection(int cfd ,int epfd)
{
    int ret = epoll_ctl(epfd ,EPOLL_CTL_DEL ,cfd ,NULL);
    if(ret==-1)
    {
        perror("epoll_ctl error");
        exit(1);
    }
    close(cfd);
}

//实现读取http协议
//cfd socket描述符
//epfd epoll事件
void http_read(int cfd ,int epfd)
{
    int len;
#if 0
    //测试打印http请求
    char buf[1024];
    len = Read(cfd ,buf ,sizeof(buf));
    if(len==0)
    {
        close_connection(cfd ,epfd);
        printf("loss connection.\n");
    }
    Write(STDOUT_FILENO ,buf ,len);
#endif
    //读取http头
    char line[1024] = {0};
    len = get_line(cfd ,line ,sizeof(buf));
    //读事件，即客户端有数据写进来
    if(len==0)
    {
        //说明客户端断开连接
        close_connection(cfd ,epfd);
        printf("loss connection.\n");
    }
    else if(len>0)
    {
        //正则表达式提取模式、路径、协议信息
        char method[16],path[256],protocol[16];
        sscanf(line ,"%[^ ] %[^ ] %[^ ]" ,method ,path ,protocol);
        printf("method=%s ,peth=%s ,prococol=%s \n",method ,path ,protocol);
        
        //处理http请求
        //判断method为GET
        if(strncasecmp(method ,"GET" ,3)==0)
        {
            //处理http请求
            http_request(cfd ,path);
        }
        // 关闭套接字, cfd从epoll上del
        //close_connection(cfd, epfd); //if closed will reset the connection
        //Write(STDOUT_FILENO ,line ,len);
    }
}

//处理http请求
//判断文件是否存在
void http_request(int cfd ,char *path)
{
    // 转码 将不能识别的中文乱码 -> 中文
    // 解码 %23 %34 %5f
    decode_str(path, path);

    //取出客户端要访问的文件名 /hello.c -> hello.c
    char *filename;
    filename = path + 1; 
    
    // 如果没有指定访问的资源, 默认显示资源目录中的内容
    if(strcmp(path, "/") == 0) {    
        // filename的值, 资源目录的当前位置
        filename = "./";
    }
    
    //查看文件类型（此处判断文件是否存在）
    //stat符号穿透，lstat不可符号穿透
    struct stat sbuf;
    int ret = stat(filename, &sbuf);
    if(ret==-1)
    {
        //文件不存在
        perror("stat error");
        //exit(1);
        //回发浏览器错误页面404
        send_error(cfd, 404, "Not Found", "NO such file or direntry"); 
    }
    else
    {
        //文件目录
        if(S_ISDIR(sbuf.st_mode))
        {
            //回发http协议应答
            send_response(cfd ,200 ,"OK" ,
                            (char *)get_file_type(".html"), //返回html形式的目录
                            sbuf.st_size); //-1或sbuf.st_size
            printf("file exists.%s\n",(const char *)get_file_type(filename));
            //回发请求数据内容
            send_dir(cfd ,(const char *)filename);
        }
        //文件
        if(S_ISREG(sbuf.st_mode))
        {
            //回发http协议应答
            send_response(cfd ,200 ,"OK" ,
                            (char *)get_file_type(filename) ,
                            //"Content-Type：text/plain; charset=iso-8859-1" ,
                            sbuf.st_size); //-1或sbuf.st_size
            printf("file exists.%s\n",(const char *)get_file_type(filename));
            //回发请求数据内容
            send_file(cfd ,(const char *)filename);
        }
    }
}

// 解析http请求消息的每一行内容
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

//发送HTTP应答
//客户端fd,错误号,错误描述,回发文件类型,文件长度
/*
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
void send_response(int cfd ,int no ,char* disp ,char *type ,int len)
{
    char buf[1024];
    //发送状态行
    sprintf(buf ,"Http1.1 %d %s\r\n" ,no ,disp);
    send(cfd ,buf ,strlen(buf) ,0);
    //发送消息报头
    sprintf(buf ,"Content-Type:%s\r\n" ,type);
    sprintf(buf+strlen(buf) ,"Content-Length:%d\r\n" ,len);
    send(cfd ,buf ,strlen(buf) ,0);
    //发送空行
    send(cfd ,"\r\n" ,2 ,0);
}

void send_file(int cfd ,const char *file)
{
    //打开文件
    int fd = open(file ,O_RDONLY);
    if(fd == -1)
    {
        perror("open error");
        exit(1);
    }
    //循环读文件并传输数据
    char buf[4096];
    ssize_t len=0 ;
    int ret=0;  
    while( (len = Read(fd ,buf ,sizeof(buf))) > 0 ) {
        //printf("read %ld ,%s\n",len ,buf);
        ret = send(cfd ,buf ,len ,0);
        //ret = Write(cfd ,buf ,len);
        memset(buf, 0, len);
        if(ret == -1)
        {
            perror("send error");
            if(errno == EAGAIN){
                //The socket is marked nonblocking and the requested operation would block
                continue;
            }else if(errno == EINTR){
                //慢速系统调用被中断
                continue;
            }else{
                exit(1);
            }
        }
        printf("send %s ,%d bytes.\n",file ,ret);
    }
    if(len == -1){
        perror("read error");
        exit(1);
    }
    close(fd);
}

const char *get_file_type(const char *filename)
{
    char *dot;

    //自右向左查找.字符，不存在则返回NULL
    dot = strrchr(filename ,'.');
    if(dot==NULL)
        return "text/plain; charset=utf-8";
    if(strcmp(dot ,"html") == 0 || strcmp(dot ,"htm") == 0)
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
    if (strcmp( dot, ".wav" ) == 0)
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
    //其余类型认为是text文本文件
    return "text/plain; charset=utf-8";
}

//显示错误页面
void send_error(int cfd, int status, char *title, char *text)
{
   char buf[4096] = {0};

	sprintf(buf, "%s %d %s\r\n", "HTTP/1.1", status, title);
	sprintf(buf+strlen(buf), "Content-Type:%s\r\n", "text/html");
	sprintf(buf+strlen(buf), "Content-Length:%d\r\n", -1);
	sprintf(buf+strlen(buf), "Connection: close\r\n");
	send(cfd, buf, strlen(buf), 0);
	send(cfd, "\r\n", 2, 0);

	memset(buf, 0, sizeof(buf));

	sprintf(buf, "<html><head><title>%d %s</title></head>\n", status, title);
	sprintf(buf+strlen(buf), "<body bgcolor=\"#cc99cc\"><h2 align=\"center\">%d %s</h4>\n", status, title);
	sprintf(buf+strlen(buf), "%s\n", text);
	sprintf(buf+strlen(buf), "<hr>\n</body>\n</html>\n");
	send(cfd, buf, strlen(buf), 0);
	
	return ; 
}
/*
// 发送目录内容
void send_dir(int cfd, const char* dirname)
{
    int i, ret;

    // 拼一个html页面<table></table>
    char buf[4094] = {0};

    sprintf(buf, "<html><head><title>目录名: %s</title></head>", dirname);
    sprintf(buf+strlen(buf), "<body><h1>当前目录: %s</h1><table>", dirname);

    char enstr[1024] = {0};
    char path[1024] = {0};
    
    // 目录项二级指针
    struct dirent** ptr;
    int num = scandir(dirname, &ptr, NULL, alphasort);
    
    // 遍历
    for(i = 0; i < num; ++i) {
    
        char* name = ptr[i]->d_name;

        // 拼接文件的完整路径
        sprintf(path, "%s/%s", dirname, name);
        printf("path = %s ===================\n", path);
        struct stat st;
        stat(path, &st);

		// 编码生成 %E5 %A7 之类的东西
        encode_str(enstr, sizeof(enstr), name);
        
        // 如果是文件
        if(S_ISREG(st.st_mode)) {       
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s\">%s</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        } else if(S_ISDIR(st.st_mode)) {		// 如果是目录       
            sprintf(buf+strlen(buf), 
                    "<tr><td><a href=\"%s/\">%s/</a></td><td>%ld</td></tr>",
                    enstr, name, (long)st.st_size);
        }
        ret = send(cfd, buf, strlen(buf), 0);
        if (ret == -1) {
            if (errno == EAGAIN) {
                perror("send error:");
                continue;
            } else if (errno == EINTR) {
                perror("send error:");
                continue;
            } else {
                perror("send error:");
                exit(1);
            }
        }
        memset(buf, 0, sizeof(buf));
        // 字符串拼接
    }

    sprintf(buf+strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);

    printf("dir message send OK!!!!\n");
#if 0
    // 打开目录
    DIR* dir = opendir(dirname);
    if(dir == NULL)
    {
        perror("opendir error");
        exit(1);
    }

    // 读目录
    struct dirent* ptr = NULL;
    while( (ptr = readdir(dir)) != NULL )
    {
        char* name = ptr->d_name;
    }
    closedir(dir);
#endif
}
*/
// "编码"，用作回写浏览器的时候，将除字母数字及/_.-~以外的字符转义后回写。
// strencode(encoded_name, sizeof(encoded_name), name);
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

void send_dir(int cfd, const char* dirname)
//int send_dir(struct bufferevent *bev,const char *dirname)
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
        send(cfd, buf, strlen(buf), 0);
        //bufferevent_write(bev, buf, strlen(buf));
        memset(buf, 0, sizeof(buf));
    }
    sprintf(buf+strlen(buf), "</table></body></html>");
    send(cfd, buf, strlen(buf), 0);
    //bufferevent_write(bev, buf, strlen(buf));
    printf("################# Dir Read OK !!!!!!!!!!!!!!\n");

    //return 0;
}
/*
 *  这里的内容是处理%20之类的东西！是"解码"过程。
 *  %20 URL编码中的‘ ’(space)
 *  %21 '!' %22 '"' %23 '#' %24 '$'
 *  %25 '%' %26 '&' %27 ''' %28 '('......
 *  相关知识html中的‘ ’(space)是&nbsp
 */
void encode_str(char* to, int tosize, const char* from)
{
    int tolen;

    for (tolen = 0; *from != '\0' && tolen + 4 < tosize; ++from) {    
        if (isalnum(*from) || strchr("/_.-~", *from) != (char*)0) {      
            *to = *from;
            ++to;
            ++tolen;
        } else {
            sprintf(to, "%%%02x", (int) *from & 0xff);
            to += 3;
            tolen += 3;
        }
    }
    *to = '\0';
}

void decode_str(char *to, char *from)
{
    for ( ; *from != '\0'; ++to, ++from  ) {     
        if (from[0] == '%' && isxdigit(from[1]) && isxdigit(from[2])) {       
            *to = hexit(from[1])*16 + hexit(from[2]);
            from += 2;                      
        } else {
            *to = *from;
        }
    }
    *to = '\0';
}

// 16进制数转化为10进制
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
