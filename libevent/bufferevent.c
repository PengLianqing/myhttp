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
#include "bufferevent.h"
#include "http_process.h"
#include "file_process.h"

//evconnlistener_new_bind回调函数
//打印连接客户端情况
//创建bufferevent事件对象，并设置回调函数
void listener_cb(
                 struct evconnlistener *listener ,
                 evutil_socket_t fd ,
                 struct sockaddr *addr ,
                 int len ,void *ptr)
{   
    //打印客户端地址结构
    char serv_ip[32];
    struct sockaddr_in *clin = (struct sockaddr_in *)addr;
    printf("new connection: client ip %s ,port: %d\n",
            inet_ntop(AF_INET,&(clin->sin_addr.s_addr),serv_ip,sizeof(serv_ip)),
            ntohs(clin->sin_port));

    //创建bufferevent事件
    struct event_base *event = ptr;
    struct bufferevent *bev;
    bev = bufferevent_socket_new(event ,fd ,BEV_OPT_CLOSE_ON_FREE);
    if (!bev)
    {
        fprintf(stderr, "Error constructing bufferevent!");
        event_base_loopbreak(event);
        return;
    }
    
    //为缓冲区设置回调函数与权限
    bufferevent_flush(bev, EV_READ | EV_WRITE, BEV_NORMAL); //清空bufferevent 要求bufferevent 强制从底层传输端口读取或者写入尽可能多的数据
    bufferevent_setcb(bev ,read_cb ,NULL ,event_cb ,NULL);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

//bufferevent回调函数
void read_cb(struct bufferevent *bev ,void *arg)
{
    //bufferevent_read
    //http_read(bev);

    char buf[4096]={0};
    char method[50], path[4096], protocol[32];
    bufferevent_read(bev, buf, sizeof(buf));
    printf("buf[%s]\n", buf);
    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol);
    printf("method[%s], path[%s], protocol[%s]\n", method, path, protocol);
    if(strcasecmp(method, "GET") == 0)
    {
        response_http(bev, method, path);
    }
    printf("******************** end call %s.........\n", __FUNCTION__);
}

void write_cb(struct bufferevent *bev ,void *arg)
{
    printf("write ok.\n");
}

void event_cb(struct bufferevent *bev , short events ,void *arg)
{
    if (events & BEV_EVENT_EOF)
    {
        printf("connection closed\n");  
                    
    }
    else if(events & BEV_EVENT_ERROR)   
    {
        printf("some other error\n");
                      
    }
    else if(events & BEV_EVENT_CONNECTED)
    {
        printf("connect success.\n");
        return;                              
    }             
    // 释放资源
    bufferevent_free(bev);
}

//延时1s后退出
void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = user_data;
    struct timeval delay = { 1, 0 };

    printf("Caught an interrupt signal; exiting cleanly in one seconds.\n");

    //event_base_dispatch将一直运行直到没有注册事件,
    //或调用event_base_loopbreak/event_base_loopexit.
    event_base_loopexit(base, &delay);
}





