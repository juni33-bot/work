/**
********************************************************************************
* Copyright (C) 2022 NEXTCHIP Inc. All rights reserved.
* This software is the confidential and proprietary information of
* NEXTCHIP, Inc. ("Confidential Information"). You shall not disclose such
* Confidential Information and shall use it only in accordance with
* the terms of the license agreement you entered into with NEXTCHIP.
********************************************************************************
*/
/**
********************************************************************************
* @file    : nc_streamer.c
*
* @brief   : Implementation of video codec stream server
*
* @author  : SoC SW team.  NextChip Inc.
*
* @date    : 2023.07.21
*
* @version : 1.0.0
********************************************************************************
* @note
* 2023.07.21 / 1.0.0 / Initial released.
*
********************************************************************************
*/
/*
********************************************************************************
*               INCLUDES
********************************************************************************
*/

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/un.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>

#include "nc_utils.h"

#define MAX_CONN_CNT 5

#define SA struct sockaddr
#define TRUE 1
#define FALSE 0
#define MAXLINE 1024
#define LISTENQ 10

volatile int keepRunning_acc = 1;
int listen_client;    // listen for clients on this socket.
pthread_t connAcceptorTid;

typedef struct
{
    int conn_fd;
    int connected;
    int needHdr;
    char ipv4_addr[INET_ADDRSTRLEN];
} stConnClientInfo;
pthread_mutex_t conn_info_mutex;
stConnClientInfo g_conn_infos[MAX_CONN_CNT] = {0,0,0,0};

void print_all_conn_infos(void)
{
    int totalcnt = 0;
    for (int i = 0; i < MAX_CONN_CNT; i++) {
        if (!g_conn_infos[i].connected) continue;
        totalcnt++;
    }
    printf("-------------------- Connected Client info --------------------\n");
    if (totalcnt < 1) {
        printf("[Connected Client] No Client Connected...\n");
    }
    else {
        for (int i = 0; i < MAX_CONN_CNT; i++) {
            if (!g_conn_infos[i].connected) continue;
            printf("%s[Connected Client %d/%d] fd(%d) ipv4addr(%s)...\n", g_conn_infos[i].needHdr?"(NEW)":"", i+1, totalcnt, g_conn_infos[i].conn_fd, g_conn_infos[i].ipv4_addr);
        }
    }
    printf("---------------------------------------------------------------\n\n");
}

void add_conn_info(int conn_fd, char* ipv4_addr)
{
    pthread_mutex_lock(&conn_info_mutex);
    for (int i = 0; i < MAX_CONN_CNT; i++) {
        if (g_conn_infos[i].conn_fd == 0 && g_conn_infos[i].connected == 0) {
            g_conn_infos[i].conn_fd = conn_fd;
            g_conn_infos[i].needHdr = 1;
            strncpy(g_conn_infos[i].ipv4_addr, ipv4_addr, (size_t)INET_ADDRSTRLEN);
            g_conn_infos[i].connected = 1;
            break;
        }
    }
    print_all_conn_infos();
    // if (!added) {
    //     printf("[Error] Can't connect anymore : max conn cnt(%d) ...\n", MAX_CONN_CNT);
    // }
    pthread_mutex_unlock(&conn_info_mutex);
}

void remove_conn_info(int conn_fd, int sync_internal)
{
    if(sync_internal) pthread_mutex_lock(&conn_info_mutex);
    for (int i = 0; i < MAX_CONN_CNT; i++) {
        if (g_conn_infos[i].conn_fd == conn_fd) {
            memset(&g_conn_infos[i], 0, sizeof(stConnClientInfo));
            break;
        }
    }
    print_all_conn_infos();
    if(sync_internal)  pthread_mutex_unlock(&conn_info_mutex);
}

int connected_client_count(void)
{
    pthread_mutex_lock(&conn_info_mutex);
    int totalcnt = 0;
    for (int i = 0; i < MAX_CONN_CNT; i++) {
        if (!g_conn_infos[i].connected) continue;
        totalcnt++;
    }
    pthread_mutex_unlock(&conn_info_mutex);
    return totalcnt;
}

void *accept_connections_task(void*)
{
    while(keepRunning_acc)
    {
        fd_set rset;
        FD_ZERO (&rset);
        FD_SET (listen_client, &rset);

        struct timeval tv;
        tv.tv_sec=0;
        tv.tv_usec=0;

        select (listen_client + 1, &rset, NULL, NULL, &tv);

        if (FD_ISSET (listen_client, &rset))
        {
            char client_header[MAXLINE];
            struct sockaddr_in cliaddr_client;
            socklen_t len = sizeof (struct sockaddr_in);
            int client_fd = accept (listen_client, (SA *)&cliaddr_client ,&len);

            if(client_fd < 0 ) {
                printf("ERROR: Error accepting connection from client\n");
            }
            else {
                if (connected_client_count() >= MAX_CONN_CNT) {
                    printf("[Error] Can't connect anymore : max conn cnt(%d) ...\n", MAX_CONN_CNT);
                    close(client_fd);
                    continue;
                }

                char ipv4_addr[INET_ADDRSTRLEN] = {0,};
                memset(ipv4_addr, 0x00, sizeof(char)*INET_ADDRSTRLEN);
                inet_ntop(AF_INET, &(cliaddr_client.sin_addr), ipv4_addr, INET_ADDRSTRLEN);

                printf ("\n-------------------------------------------------------------------------------\n");
                printf ("Accepted connection from client(ip: %s, port:%d, sockfd: %d) ...\n", ipv4_addr, cliaddr_client.sin_port, client_fd);
                printf ("-------------------------------------------------------------------------------\n");

                int bytes_read = (int)recv (client_fd , client_header, MAXLINE, 0);
                if (bytes_read < 0)
                {
                    printf ("Mount point not received from client ...\n");
                    exit (1);
                } else if(bytes_read == 0)
                {
                    printf("----- Socket close\n");
                }

                // printf ("\n====== Connected Client Info ======\n");
                // printf ("Client header : \n%s", client_header);
                // printf ("Client num = %d\n", client_fd);
                // printf ("===================================\n\n");

                char videoBuffer[MAXLINE];
                strcpy (videoBuffer, "HTTP/1.0 200 OK\r\n");
                strcat (videoBuffer, "Content-Type: video/mp4\r\n");
                strcat (videoBuffer, "\r\n");

                // Send the HTTP Acknowledgement
                if (send (client_fd, videoBuffer, strlen (videoBuffer)+1, MSG_NOSIGNAL) < 0)
                {
                    perror ("send of HTTP Ack failure");
                }

                add_conn_info(client_fd, ipv4_addr);

                usleep(100*1000);
            }
        }

        FD_SET(listen_client, &rset);
    }
    return NULL;
}

void nc_start_stream_server(int port)
{
    // initialize
    int thr_id;
    pthread_mutex_init(&conn_info_mutex, NULL);

    // Create a TCP server socket to listen to client
    listen_client = socket (AF_INET, SOCK_STREAM, 0);
    if (listen_client < 0)
    {
        printf ("ERROR: client-socket creation error ...\n");
        exit (1);
    }

#if 1
    int val = 1;
    int ret = setsockopt(listen_client, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
    if (ret < 0 )
    {
        printf ("ERROR: setsockopt(SO_REUSEADDR) error(%d) ...\n", errno);
        exit (2);
    }
#endif

    struct sockaddr_in servaddr_client;
    bzero (&servaddr_client, sizeof (servaddr_client));

    servaddr_client.sin_family = AF_INET;
    servaddr_client.sin_addr.s_addr = htonl (INADDR_ANY);
    servaddr_client.sin_port = htons ((uint16_t)port);

    int bind_client = bind (listen_client, (SA *) &servaddr_client, sizeof (SA));
    if (bind_client < 0 )
    {
        printf ("ERROR: client-socket bind error ...\n");
        exit (3);
    }

    listen (listen_client, LISTENQ);   // Make the socket passive

    thr_id = pthread_create (&connAcceptorTid, NULL, (void * (*) (void *))accept_connections_task, NULL);
    if (thr_id < 0) {
        perror("thread create error : ");
    }
    printf("[%s] OK!!!\n", __FUNCTION__);
}

void nc_shutdown_stream_server(void)
{
    keepRunning_acc = 0;
    usleep(500 * 1000);

    printf("\nshutdown accept socket ...\n");
    // close (listen_client);
    shutdown(listen_client, SHUT_RDWR);

    pthread_mutex_lock(&conn_info_mutex);
    for (int i = 0; i < MAX_CONN_CNT; i++) {
        if (g_conn_infos[i].connected) {
            // printf("close connection : fd(%d) ip(%s)\n", g_conn_infos[i].conn_fd, g_conn_infos[i].ipv4_addr);
            // close(g_conn_infos[i].conn_fd);
            shutdown(g_conn_infos[i].conn_fd, SHUT_RDWR);
        }
    }
    pthread_mutex_unlock(&conn_info_mutex);
}

// #define MEASURE_SNDTIME
int nc_push_stream_data(unsigned char *ptr_send_data, int send_data_size, int with_header, unsigned char *ptr_header, int header_size)
{
    int ret = -1;

#ifdef MEASURE_SNDTIME
    uint64_t start_time = 0;
    uint64_t elapsed_ms = 0;
    start_time = nc_get_mono_time();
#endif
    pthread_mutex_lock(&conn_info_mutex);
    for (int i = 0; i < MAX_CONN_CNT; i++) {
        if (!g_conn_infos[i].connected) continue;

        if(g_conn_infos[i].needHdr || with_header)
        {
            g_conn_infos[i].needHdr = 0;

            // printf("Start new streaming to '%s' [fd:%d] hdr_ptr 0x%08x ret %d bytes\n", g_conn_infos[i].ipv4_addr, g_conn_infos[i].conn_fd, ptr_header, header_size);
            ret = (int)send(g_conn_infos[i].conn_fd, ptr_header, header_size, MSG_NOSIGNAL);
        }
        ret = (int)send(g_conn_infos[i].conn_fd, ptr_send_data, send_data_size, MSG_NOSIGNAL);

        if (ret >= 0) {
            // printf("[Send OK] g_conn_infos[%d].conn_fd(%d) -> send %d bytes\n", i, g_conn_infos[i].conn_fd, ret);
        }
        if (ret < 0) { // on error, return -1
            // printf("[Error] g_conn_infos[%d].conn_fd(%d) -> errno(%d):%s\n", i, g_conn_infos[i].conn_fd, errno, strerror(errno));
            // if (errno == ECONNRESET) {
            //     // connection reset by peer
            // }
            // else if (errno == EPIPE) {
            //     // broken pipe
            // }
            // else {
            //     // TBD
            // }

            // remove connection
            printf("\nClient %d (ip:%s, fd:%d) disconnected...\n", i+1, g_conn_infos[i].ipv4_addr, g_conn_infos[i].conn_fd);
            close(g_conn_infos[i].conn_fd);
            remove_conn_info(g_conn_infos[i].conn_fd, FALSE);
        }
    }
    pthread_mutex_unlock(&conn_info_mutex);

#ifdef MEASURE_SNDTIME
    elapsed_ms = nc_elapsed_time(start_time);
    if (elapsed_ms > 10) {
        printf("(%s) elapsed time : %llu ms\n", __FUNCTION__, elapsed_ms);
    }
#endif

    return ret;
}