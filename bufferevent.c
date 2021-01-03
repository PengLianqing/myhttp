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

/**
  * @brief          bufferevent的读回调函数
  * 1）读取http请求；
  * 2）做出http响应response_http()。
  * @param[in]      内核自动传入,bev
  * @retval         NULL
  */
void read_cb(struct bufferevent *bev ,void *arg)
{
    //从http请求中获取method、path信息
    char buf[4096]={0};
    char method[50], path[4096], protocol[32];
    bufferevent_read(bev, buf, sizeof(buf));
    printf("buf[%s]\n", buf);
    sscanf(buf, "%[^ ] %[^ ] %[^ \r\n]", method, path, protocol); //正则表达式获取信息

    //http响应
    printf("method[%s], path[%s], protocol[%s]\n", method, path, protocol);
    if(strcasecmp(method, "GET") == 0)
    {
        response_http(bev, method, path);
    }
    printf("******************** end call %s.........\n", __FUNCTION__);
}

/**
  * @brief          bufferevent的事件回调函数
  * event响应，如果是BEV_EVENT_CONNECTED则返回，发生错误或连接断开则释放资源。
  * @param[in]      内核自动传入,bev,events
  * @retval         NULL
  */
void event_cb(struct bufferevent *bev , short events ,void *arg)
{
    if (events & BEV_EVENT_EOF)
    {
        printf("connection closed\n");  //遇到文件结束指示，表示连接断开
                    
    }
    else if(events & BEV_EVENT_ERROR)   //操作时发生错误
    {
        printf("some other error\n");
                      
    }
    else if(events & BEV_EVENT_CONNECTED) //请求的连接过程已经完成
    {
        printf("connect success.\n");
        return;                              
    } 

    // 释放资源
    bufferevent_free(bev);
}

/**
  * @brief          信号事件回调函数
  * 接收到信号如ctrl+c，此时延时1s后退出程序。
  * @param[in]      内核自动传入,bev,events
  * @retval         NULL
  */
void signal_cb(evutil_socket_t sig, short events, void *user_data)
{
    struct event_base *base = user_data;
    struct timeval delay = { 1, 0 };

    printf("Caught an interrupt signal; exiting cleanly in one seconds.\n");

    //event_base_dispatch将一直运行直到没有注册事件,
    //或调用event_base_loopbreak/event_base_loopexit.
    event_base_loopexit(base, &delay);
}





