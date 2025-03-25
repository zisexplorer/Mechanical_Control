/**
 ****************************************************************************************************
 * @file        lwip_demo
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-04
 * @brief       lwIP Netconn TCPServer 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 探索者 F407开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */
 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include <stdint.h>
#include <stdio.h>
//#include "./BSP/LCD/lcd.h"
//#include "./MALLOC/malloc.h"
#include <lwip/sockets.h>
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwiptcpdemo.h"	
#ifndef HTTPD_DEBUG
#define HTTPD_DEBUG         LWIP_DBG_OFF
#endif
static const char http_html_hdr[] = "HTTP/1.1 200 OK\r\nContent-type: text/html\r\n\r\n";

static const char http_index_html[] = "<html><head><title>Congrats!</title></head>\
                                       <body><h1 align=\"center\">Hello World!</h1>\
                                       <h2 align=\"center\">Welcome to Fire lwIP HTTP Server!</h1>\
                                       <p align=\"center\">This is a small test page, served by httpserver-netconn.</p>\
                                       <p align=\"center\"><a href=\"http://www.firebbs.cn/forum.php/\"> <font size=\"6\"> 野火电子论坛 </font> </a></p>\
                                       <a href=\"http://www.firebbs.cn/forum.php/\">\
                                       <p align=\"center\"><img src=\"http://www.firebbs.cn/data/attachment/portal/201806/05/163015rhz7mbgbt0zfujzh.jpg\" /></a>\
                                       </body></html>";

/** Serve one HTTP connection accepted in the http thread */    //http://www.firebbs.cn/data/attachment/portal/201806/05/163015rhz7mbgbt0zfujzh.jpg
static void
http_server_netconn_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  char *buf;
  u16_t buflen;
  err_t err;
  
  /* Read the data from the port, blocking if nothing yet there. 
   We assume the request (the part we care about) is in one netbuf */
  err = netconn_recv(conn, &inbuf);
  
  if (err == ERR_OK) {
    netbuf_data(inbuf, (void**)&buf, &buflen);
    
    /* Is this an HTTP GET command? (only check the first 5 chars, since
    there are other formats for GET, and we're keeping it very simple )*/
    if (buflen>=5 &&
        buf[0]=='G' &&
        buf[1]=='E' &&
        buf[2]=='T' &&
        buf[3]==' ' &&
        buf[4]=='/' ) {
      
      /* Send the HTML header 
             * subtract 1 from the size, since we dont send the \0 in the string
             * NETCONN_NOCOPY: our data is const static, so no need to copy it
       */
//					printf("%s",http_html_hdr);
//					printf("%s",http_index_html);
      netconn_write(conn, http_html_hdr, sizeof(http_html_hdr)-1, NETCONN_COPY);
//      
//      /* Send our HTML page */
      netconn_write(conn, http_index_html, sizeof(http_index_html)-1, NETCONN_COPY);
					printf("%s",http_html_hdr);
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);
  
  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}

/* TCPServer 不需要设置远程IP地址 */
//#define DEST_IP_ADDR0               192
//#define DEST_IP_ADDR1               168
//#define DEST_IP_ADDR2                 0
//#define DEST_IP_ADDR3                 

#define LWIP_DEMO_RX_BUFSIZE         2000  /* 最大接收数据长度 */
#define LWIP_DEMO_PORT               80  /* 连接的本地端口号 */

/* 接收数据缓冲区 */
//uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE]; 
/* 发送数据内容 */
char *g_lwip_demo_sendbuf = "ALIENTEK DATA \r\n";
/* 数据发送标志位 */
uint8_t g_lwip_send_flag;

extern QueueHandle_t g_display_queue;     /* 显示消息队列句柄 */

/**
 * @brief       lwip_demo实验入口
 * @param       无
 * @retval      无
 */
void lwip_tcp_server_demo(void)
{
    static struct netconn *tcp_serverconn = NULL; /* TCP SERVER网络连接结构体 */
    uint32_t  data_len = 0;
    struct    pbuf *q;
    err_t     err,recv_err;
    uint8_t   remot_addr[4];
    struct netconn *newconn;
    static    ip_addr_t ipaddr;
    static    u16_t  port;
    BaseType_t lwip_err;
    char *tbuf;
    
   
    
    /* 第一步：创建一个TCP控制块 */
    tcp_serverconn = netconn_new(NETCONN_TCP);                      /* 创建一个TCP链接 */
    /* 第二步：绑定TCP控制块、本地IP地址和端口号 */
    netconn_bind(tcp_serverconn,IP_ADDR_ANY,LWIP_DEMO_PORT);        /* 绑定端口 8080号端口 */
    /* 第三步：监听 */
    netconn_listen(tcp_serverconn);                                 /* 进入监听模式 */
//    tcp_serverconn->recv_timeout = 10;                              /* 禁止阻塞线程 等待10ms */
    
    while (1) 
    {
			do {
				err = netconn_accept(tcp_serverconn, &newconn);
				if (err == ERR_OK) {
//					taskENTER_CRITICAL();   
				http_server_netconn_serve(newconn);
//					taskEXIT_CRITICAL();  
				netconn_delete(newconn);
				}
			} while(err == ERR_OK);
		
			netconn_close(tcp_serverconn);
			netconn_delete(tcp_serverconn);
        /* 第四步：接收连接请求 */
//        err = netconn_accept(tcp_serverconn,&newconn);              /* 接收连接请求 */
////        if (err == ERR_OK) newconn->recv_timeout = 10;

//        if (err == ERR_OK)                                          /* 处理新连接的数据 */
//        { 
//            struct netbuf *recvbuf;
//            netconn_getaddr(newconn,&ipaddr,&port,0);               /* 获取远端IP地址和端口号 */
//            
//            remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
//            remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
//            remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
//            remot_addr[0] = (uint8_t)(ipaddr.addr);
//            printf("主机%d.%d.%d.%d连接上服务器,主机端口号为:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
//           
//            
//            while (1)
//            {

//                
//                if ((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)           /* 接收到数据 */
//                { 
//                    taskENTER_CRITICAL();                                           /* 进入临界区 */
//                    memset(g_lwip_demo_recvbuf,0,LWIP_DEMO_RX_BUFSIZE);               /* 数据接收缓冲区清零 */


//										http_server_netconn_serve(newconn);

//                    taskEXIT_CRITICAL();                                /* 退出临界区 */
//                    data_len = 0;                                       /* 复制完成后data_len要清零 */
//                    
//                   
//                    
//										printf("tcp server :%s \r\n",g_lwip_demo_recvbuf);
//                  
//                    
//                    netbuf_delete(recvbuf);
//                }
//                else if (recv_err == ERR_CLSD)                           /* 关闭连接 */
//                {
//                    netconn_close(newconn);
//                    netconn_delete(newconn);
//                    printf("主机:%d.%d.%d.%d断开与服务器的连接\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
//               
//                    break;
//                }
//            }
//        }
    }
}
