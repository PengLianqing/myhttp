/**
  ****************************(C) COPYRIGHT 2021 Peng****************************
  * @file       main.c
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
#include "main.h"
//user bufferevent callback
#include "bufferevent.h"

//#define SERVER_PORT 6202

//服务器端程序
int main(int argc,char **argv)
{
    //切换到输入参数指向的目录
    if(argc < 3)
    {
        printf("./event_http port path\n");
        return -1;
    }
    if(chdir(argv[2]) < 0) {
        printf("dir is not exists: %s\n", argv[2]);
        perror("chdir err:");
        return -1;
    }

    //初始化服务器地址结构
    struct sockaddr_in serv;
    memset(&serv ,0 ,sizeof(serv));
    serv.sin_family = AF_INET;
    //serv.sin_port = htons(SERVER_PORT);
    serv.sin_port = htons( atoi(argv[1]) ); //atoi字符串转整形，htons整形转网络字节序
    serv.sin_addr.s_addr = htonl(INADDR_ANY);

    //创建eventbase
    struct event_base *base;
    base = event_base_new();
    if(!base)
    {
        fprintf(stderr, "Could not initialize libevent!\n");
        return 1;
    }

    //创建listener
    struct evconnlistener *listener;
    listener = evconnlistener_new_bind(base ,listener_cb ,(void *)base ,
                                       LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE ,
                                       -1 ,(struct sockaddr *)&serv ,sizeof(serv));
    if(!listener)
    {
        fprintf(stderr, "Could not create a listener!\n");
        return 1;
    }

    //创建信号事件，捕捉并处理
    //evsignal_new，不同于event_new
    struct event *signal_event;
    signal_event = evsignal_new(base ,SIGINT ,signal_cb ,(void *)base);
    if (!signal_event || event_add(signal_event, NULL)<0) 
    {
        fprintf(stderr, "Could not create/add a signal event!\n");
        return 1;
    }

    printf("##############start task.\n");
    //启动循环
    event_base_dispatch(base);

    //释放listener与base
    evconnlistener_free(listener);
    event_free(signal_event);
    event_base_free(base);

    printf("All done.\n");
    return 0;
}