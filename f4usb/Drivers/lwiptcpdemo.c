/**
 ****************************************************************************************************
 * @file        lwip_demo
 * @author      ����ԭ���Ŷ�(ALIENTEK)
 * @version     V1.0
 * @date        2020-04-04
 * @brief       lwIP Netconn TCPServer ʵ��
 * @license     Copyright (c) 2020-2032, ������������ӿƼ����޹�˾
 ****************************************************************************************************
 * @attention
 *
 * ʵ��ƽ̨:����ԭ�� ̽���� F407������
 * ������Ƶ:www.yuanzige.com
 * ������̳:www.openedv.com
 * ��˾��ַ:www.alientek.com
 * �����ַ:openedv.taobao.com
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
                                       <p align=\"center\"><a href=\"http://www.firebbs.cn/forum.php/\"> <font size=\"6\"> Ұ�������̳ </font> </a></p>\
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

/* TCPServer ����Ҫ����Զ��IP��ַ */
//#define DEST_IP_ADDR0               192
//#define DEST_IP_ADDR1               168
//#define DEST_IP_ADDR2                 0
//#define DEST_IP_ADDR3                 

#define LWIP_DEMO_RX_BUFSIZE         2000  /* ���������ݳ��� */
#define LWIP_DEMO_PORT               80  /* ���ӵı��ض˿ں� */

/* �������ݻ����� */
//uint8_t g_lwip_demo_recvbuf[LWIP_DEMO_RX_BUFSIZE]; 
/* ������������ */
char *g_lwip_demo_sendbuf = "ALIENTEK DATA \r\n";
/* ���ݷ��ͱ�־λ */
uint8_t g_lwip_send_flag;

extern QueueHandle_t g_display_queue;     /* ��ʾ��Ϣ���о�� */

/**
 * @brief       lwip_demoʵ�����
 * @param       ��
 * @retval      ��
 */
void lwip_tcp_server_demo(void)
{
    static struct netconn *tcp_serverconn = NULL; /* TCP SERVER�������ӽṹ�� */
    uint32_t  data_len = 0;
    struct    pbuf *q;
    err_t     err,recv_err;
    uint8_t   remot_addr[4];
    struct netconn *newconn;
    static    ip_addr_t ipaddr;
    static    u16_t  port;
    BaseType_t lwip_err;
    char *tbuf;
    
   
    
    /* ��һ��������һ��TCP���ƿ� */
    tcp_serverconn = netconn_new(NETCONN_TCP);                      /* ����һ��TCP���� */
    /* �ڶ�������TCP���ƿ顢����IP��ַ�Ͷ˿ں� */
    netconn_bind(tcp_serverconn,IP_ADDR_ANY,LWIP_DEMO_PORT);        /* �󶨶˿� 8080�Ŷ˿� */
    /* ������������ */
    netconn_listen(tcp_serverconn);                                 /* �������ģʽ */
//    tcp_serverconn->recv_timeout = 10;                              /* ��ֹ�����߳� �ȴ�10ms */
    
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
        /* ���Ĳ��������������� */
//        err = netconn_accept(tcp_serverconn,&newconn);              /* ������������ */
////        if (err == ERR_OK) newconn->recv_timeout = 10;

//        if (err == ERR_OK)                                          /* ���������ӵ����� */
//        { 
//            struct netbuf *recvbuf;
//            netconn_getaddr(newconn,&ipaddr,&port,0);               /* ��ȡԶ��IP��ַ�Ͷ˿ں� */
//            
//            remot_addr[3] = (uint8_t)(ipaddr.addr >> 24); 
//            remot_addr[2] = (uint8_t)(ipaddr.addr>> 16);
//            remot_addr[1] = (uint8_t)(ipaddr.addr >> 8);
//            remot_addr[0] = (uint8_t)(ipaddr.addr);
//            printf("����%d.%d.%d.%d�����Ϸ�����,�����˿ں�Ϊ:%d\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3],port);
//           
//            
//            while (1)
//            {

//                
//                if ((recv_err = netconn_recv(newconn,&recvbuf)) == ERR_OK)           /* ���յ����� */
//                { 
//                    taskENTER_CRITICAL();                                           /* �����ٽ��� */
//                    memset(g_lwip_demo_recvbuf,0,LWIP_DEMO_RX_BUFSIZE);               /* ���ݽ��ջ��������� */


//										http_server_netconn_serve(newconn);

//                    taskEXIT_CRITICAL();                                /* �˳��ٽ��� */
//                    data_len = 0;                                       /* ������ɺ�data_lenҪ���� */
//                    
//                   
//                    
//										printf("tcp server :%s \r\n",g_lwip_demo_recvbuf);
//                  
//                    
//                    netbuf_delete(recvbuf);
//                }
//                else if (recv_err == ERR_CLSD)                           /* �ر����� */
//                {
//                    netconn_close(newconn);
//                    netconn_delete(newconn);
//                    printf("����:%d.%d.%d.%d�Ͽ��������������\r\n",remot_addr[0], remot_addr[1],remot_addr[2],remot_addr[3]);
//               
//                    break;
//                }
//            }
//        }
    }
}
