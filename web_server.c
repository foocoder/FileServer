/*************************************************************************
  > File Name: web_server.c
  > Author: DuanFuchen
  > Mail: slow295185031@163.com
  > Created Time: 2014年12月21日 星期日 18时55分34秒
 ************************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h> //sockaddr_in
/*
    struct in_addr{
        in_addr_t s_addr; // 32-bit IPv4 address
                          // network byte ordered
    }

    struct sockaddr_in{
        uint8_t          sin_len;        //length of structure(16)
        sa_family_t      sin_family;     //AF_INET
        in_port_t        sin_port;       //16-bit TCP or UDP port number
                                         //network byte ordered
        struct in_addr   sin_addr;       //32-bit IPv4 address
                                         //network byte orderd
        char             sin_zero[8];    //unused
    }
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <arpa/inet.h>
/*#include <pthread.h>*/
#include "server_options.h"

#define MAX_SOCK_FD FD_SETSIZE
#define PORT 8082
#define BUFFER_SIZE 1024
#define MAX_QUE_CONN_NM 5
#define MAX_LEN 1024
#define FILE_BUF_LEN  10240

#define FILE_OK 200
#define DIR_OK 100
#define FILE_NOT_FOUND 404
#define FILE_FORBIDEN 403

void get_URI(char * recv_buf, char * uri_buf)
{
    //解析消息中URI
    char * p;
    p = strtok(recv_buf, " ");
    p = strtok(NULL, " ");
    strcpy(uri_buf, p);
}

void url_decode(char *pURL)
{
    //URL中文解码
    int i = 0;
    while(*(pURL + i))
    {
        if((*pURL = *(pURL+i)) == '%')
        {
            *pURL = *(pURL+i+1) >= 'A' ? ((*(pURL+i+1) & 0xDF) - 'A') + 10 : (*(pURL+i+1) - '0');
            *pURL = (*pURL) * 16;
            *pURL += *(pURL+i+2) >= 'A' ? ((*(pURL+i+2) & 0xDF) - 'A') + 10 : (*(pURL+i+2) - '0');
            i += 2;
        }
        else if (*(pURL+i) == '+')
        {
            *pURL = ' ';
        }
        pURL++;
    }
    *pURL = '\0';
}

int get_URI_STATUS(char * uri_buf, char * rootcwd, char * currentcwd)
{
    //获得请求状态
    char path[BUFFER_SIZE];
    struct stat fileinfo;

    strcpy(path, rootcwd);
    strcat(path, uri_buf);
    stat(path, &fileinfo);

    if (access(path, F_OK) == -1)
    {
        fprintf(stderr, "File : %s not Found!\n",path);
        return FILE_NOT_FOUND;
    }
    if (access(path, R_OK) == -1)
    {
        fprintf(stderr, "File : %s FORBIDEN!\n",path);
        return FILE_FORBIDEN;
    }
    if (S_ISDIR(fileinfo.st_mode))
    {
        if(chdir(path) == -1)
        {
            fprintf(stderr, "Error in Chdir to Path: %s\n", path);
        }
        getcwd(currentcwd, MAX_LEN);
        return DIR_OK;
    }
    return FILE_OK;
}

void get_fileType(char *filename, char *fileType)
{
    //解析文件格式
    if (strstr(filename, ".html"))
        strcpy(fileType, "text/html");
    else if (strstr(filename, ".htm"))
        strcpy(fileType, "text/html");
    else if (strstr(filename, ".ico"))
        strcpy(fileType, "image/x-icon");
    else if (strstr(filename, ".png"))
        strcpy(fileType, "image/png");
    else if (strstr(filename, ".gif"))
        strcpy(fileType, "image/gif");
    else if (strstr(filename, ".mp3") || strstr(filename, ".MP3"))
        strcpy(fileType, "application/octet-stream");
    else if (strstr(filename, ".wav") || strstr(filename, ".WAV"))
        strcpy(fileType, "application/octet-stream");
    else
        strcpy(fileType, "text/plain");
}

long send_file(int fd, char * rootcwd, char * uri_buf, char * fileType)
{
    //传输文件
    char msg_head[BUFFER_SIZE], msg_body_buf[FILE_BUF_LEN];
    char path[MAX_LEN], filename[MAX_LEN];
    char *p;
    struct stat statbuf;
    unsigned long filesize = -1;
    int src_file, read_length;

    strcpy(path, rootcwd);
    strcat(path, uri_buf);
    if (stat(path, &statbuf) < 0)
    {
        return -1;
    }
    else
    {
        filesize = statbuf.st_size;
    }

    sprintf(msg_head, "HTTP/1.1 200 OK\r\n");
    sprintf(msg_head, "%sServer: Web Server\r\n", msg_head);
    sprintf(msg_head, "%sContent-Type:%s\r\n", msg_head, fileType);
    sprintf(msg_head, "%sContent-Length:%ld\r\n", msg_head, filesize);
    if (strstr(fileType, "application/octet-stream") || strstr(fileType, "text/plain"))
    {
        p = strtok(uri_buf, "/");
        while (p)
        {
            strcpy(filename, p);
            p = strtok(NULL, "/");
        }
        sprintf(msg_head, "%sContent-Disposition:attachment;filename=%s\r\n\r\n", msg_head, filename);
    }
    else
        sprintf(msg_head, "%s\r\n", msg_head);

    if (send(fd, msg_head, strlen(msg_head), 0) == -1)
    {
        perror("Error in send_file to Send Head!\n");
        return -1;
    }

    if ((src_file = open(path, O_RDONLY)) < 0)
    {
        perror("Open file Error!\n");
        return -1;
    }

    memset(msg_body_buf, 0, sizeof(msg_body_buf));
    while((read_length = read(src_file, msg_body_buf, sizeof(msg_body_buf))) > 0)
    {
        if (send(fd, msg_body_buf, read_length, 0) == -1)
        {
            perror("Error in send_file to Send Body!\n");
            return -1;
        }
    }
    close(src_file);
    return filesize;
}

int display_error(int fd, int error_no, char * uri_buf)
{
    //发送错误信息
    char msg_head[BUFFER_SIZE], msg_txt[BUFFER_SIZE];
    switch (error_no)
    {
        case FILE_NOT_FOUND:
            //编辑相应消息报头
            sprintf(msg_head, "HTTP/1.1 404 Not Found\r\n");
            sprintf(msg_head, "%sServer: Web Server\r\n",msg_head);
            sprintf(msg_head, "%sContent-Type:text/htmlcharset=utf-8\r\n",msg_head);
            //编辑响应消息内容
            sprintf(msg_txt, "<html><head><title>404 Not Found</title>");
            sprintf(msg_txt, "%s<link rel=\"shortcut icon\" href=\"404.png\" type=\"image/x-icon\"/>", msg_txt);
            sprintf(msg_txt, "%s<link rel=\"icon\" href=\"404.png\" type=\"image/x-icon\"/></head>", msg_txt);
            sprintf(msg_txt, "%s<body><h1>404 Not Found</h1>",msg_txt);
            sprintf(msg_txt, "%s<p>The Request Rescouse %s Coundn't Be Found!</p></body></html>",msg_txt, uri_buf);
            break;
        case FILE_FORBIDEN:
            //编辑相应消息报头
            sprintf(msg_head, "HTTP/1.1 403 Forbidden\r\n");
            sprintf(msg_head, "%sServer: Web Server\r\n",msg_head);
            sprintf(msg_head, "%sContent-Type:text/html;charset=utf-8\r\n",msg_head);
            //编辑响应消息内容
            sprintf(msg_txt, "<html><head><title>403 Read Forbiden</title>");
            sprintf(msg_txt, "%s<link rel=\"shortcut icon\" href=\"403.png\" type=\"image/x-icon\"/>", msg_txt);
            sprintf(msg_txt, "%s<link rel=\"icon\" href=\"403.png\" type=\"image/x-icon\"/></head>", msg_txt);
            sprintf(msg_txt, "%s<body><h1>403 Read Forbiden</h1>",msg_txt);
            sprintf(msg_txt, "%s<p>The Request Rescouse %s is Forbiden!</p></body></html>",msg_txt, uri_buf);
            break;
        default:
            break;
    }
    sprintf(msg_head,"%sContent-Length:%d\r\n\r\n",msg_head,(int) strlen(msg_txt));
    if (send(fd, msg_head, strlen(msg_head), 0) == -1)
    {
        perror("Error in display_error to Send Head!\n");
        return -1;
    }
    if (send(fd, msg_txt, strlen(msg_txt), 0) == -1)
    {
        perror("Error in display_error to Send Txt!\n");
        return -1;
    }
    return 0;
}


int main(int argc, char * argv[])
{
    struct sockaddr_in server_sockaddr,client_sockaddr;
    struct timeval timeout;
    int sin_size,recvbytes;
    int listenfd,clientfd,fd;
    int nready, client[FD_SETSIZE];
    int i, maxi, maxfd;
    char recv_buf[BUFFER_SIZE],send_buf[BUFFER_SIZE],uri_buf[BUFFER_SIZE];
    char rootcwd[MAX_LEN], fileType[MAX_LEN], currentcwd[MAX_LEN];
    fd_set allset, rset;
    /*pthread_t thread[2];*/
    int res,uri_status;
    void * thrd_ret;

    if (argc < 2)
    {
        printf("USAGE: ./server path\n");
        return -1;
    }

    if (chdir(argv[1]) == -1)
    {
        printf("Error in Share Dir %s\n", argv[1]);
        exit(-1);
    }
    //建立socket链接TCP协议
    if ((listenfd = socket(AF_INET,SOCK_STREAM,0)) == -1)
    {
        perror("Open socket failed!\n");
        exit(1);
    }
    printf("OPEN SOCKET id = %d \n",listenfd);

    //设置socketaddr_in结构体数据
    server_sockaddr.sin_family = AF_INET;
    server_sockaddr.sin_port = htons(PORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_sockaddr.sin_zero),8);

    int j = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &j ,sizeof(j));

    //Bind
    if (bind(listenfd,(struct sockaddr*)&server_sockaddr,sizeof(struct sockaddr)) == -1)
    {
        perror("ERROR in bind!\n");
        exit(1);
    }
    printf("Bind suceessful!\n");

    //Listen 监听信息
    if (listen(listenfd, MAX_QUE_CONN_NM) == -1)
    {
        perror("Error in Listen!\n");
        exit(1);
    }
    printf("Listening......\n");

    maxfd = listenfd;
    maxi  = -1;
    for(i=0; i<FD_SETSIZE; ++i){
        client[i] = -1;
    }
    getcwd(rootcwd, MAX_LEN);
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    while (1)
    {

        getcwd(currentcwd, MAX_LEN);
        rset = allset;
        sin_size = sizeof(struct sockaddr_in);
        memset(recv_buf, 0, sizeof(recv_buf));

        //UNIX Network API
        //rset = allset;
        //printf("select...\n");
        //nready = select(maxfd+1, &rset, NULL, NULL, NULL);

        //if(FD_ISSET(listenfd, &rset)){
        //    clientfd = accept(listenfd, (struct sockaddr *) &client_sockaddr, &sin_size);
        //    for(i=0; i<FD_SETSIZE; ++i) {
        //        if(client[i]<0){
        //            client[i] = clientfd;
        //            break;
        //        }
        //    }
        //    if(i == FD_SETSIZE){
        //        perror("Too many clients\n");
        //        exit(-1);
        //    }
        //    FD_SET(clientfd, &allset);
        //    if(clientfd > maxfd)
        //        maxfd = clientfd;
        //    if(i > maxi)
        //        maxi = i;
        //    if(--nready <=0)
        //        continue;
        //}

        //for(i=0; i<=maxi; ++i){
        //    int sockfd;
        //    if( (sockfd=client[i]) < 0 ){
        //        continue;
        //    }
        //    if(FD_ISSET(sockfd, &rset)){
        //        if((recvbytes = recv(sockfd, &recv_buf, BUFFER_SIZE, 0)) ==0){
        //            close(sockfd);
        //            FD_CLR(sockfd, &allset);
        //            client[i] = -1;
        //            printf(" %d fd is leaving\n", sockfd);
        //        }
        //        else //客户端发送消息
        //        {
        //            printf("\nHTTP header:\n %s \n", recv_buf);
        //            if (!strstr(recv_buf, "HTTP/")) //非HTTP协议
        //            {
        //                fprintf(stderr,"Not HTTP Protocol!\n");
        //                /*close(sockfd);*/
        //                /*FD_CLR(sockfd, &allset);*/
        //                /*exit(-1);*/
        //            }
        //            else
        //            {
        //                get_URI(recv_buf, uri_buf);
        //                url_decode(uri_buf);
        //                printf("uri_buf %s\n",uri_buf);
        //                if(!strcmp(uri_buf, "/favicon.ico")){
        //                    ///*[>Server 主动关闭链接<]*/
        //                    /*close(sockfd);*/
        //                    /*FD_CLR(sockfd, &allset);*/
        //                    continue;
        //                }
        //                uri_status = get_URI_STATUS(uri_buf, rootcwd, currentcwd);
        //                printf("Status:%d\n", uri_status);
        //                switch (uri_status)
        //                {
        //                    case FILE_OK:
        //                        get_fileType(uri_buf, fileType);
        //                        send_file(sockfd, rootcwd, uri_buf, fileType);
        //                        /*close(sockfd);*/
        //                        /*FD_CLR(sockfd, &allset);*/
        //                        break;
        //                    case DIR_OK:
        //                     //   [>display_dir(sockfd, rootcwd, currentcwd, uri_buf);<]
        //                        printDir(sockfd, rootcwd, uri_buf);
        //                        /*close(sockfd);*/
        //                        /*FD_CLR(sockfd, &allset);*/
        //                        break;
        //                    case FILE_NOT_FOUND:
        //                        if ((deteleFiles(sockfd, rootcwd, uri_buf)  != 0)&&
        //                                (getRenameInfo(sockfd, rootcwd, uri_buf)!= 0))
        //                        {
        //                            printf("file no found\n");
        //                        }
        //                        /*close(sockfd);*/
        //                        /*FD_CLR(sockfd, &allset);*/
        //                        break;
        //                    case FILE_FORBIDEN:
        //                        /*close(sockfd);*/
        //                        /*FD_CLR(sockfd, &allset);*/
        //                        break;
        //                    default:
        //                        break;
        //                }
        //            }
        //        }
        //    }
        //    if(--nready <=0)
        //        break;
        //}

        /*accept 阻塞式*/
        /*printf("Waiting accept...\n");*/
        /*if(( clientfd = accept(listenfd, (struct sockaddr *) & client_sockaddr, &sin_size)) == -1){*/
            /*perror("Error in Accept");*/
        /*}*/
        /*printf("New connection %d from %s\n",clientfd,inet_ntoa(client_sockaddr.sin_addr));*/
        /*if ((recvbytes = recv(fd, &recv_buf, BUFFER_SIZE, 0)) <= 0) //客户端关闭链接*/
        /*{*/
            /*close(fd);*/
            /*FD_CLR(fd, &allset);*/
            /*printf("Client %d has left\n",fd);*/
        /*}*/
        /*else //客户端发送消息*/
        /*{*/
            /*//打印请求报头*/
            /*printf("\nHTTP header;\n %s \n", recv_buf);*/
            /*if (!strstr(recv_buf, "HTTP/")) //非HTTP协议*/
            /*{*/
                /*fprintf(stderr,"Not HTTP Protocol!\n");*/
                /*close(fd);*/
                /*FD_CLR(fd, &allset);*/
                /*exit(-1);*/
            /*}*/
            /*else*/
            /*{*/
                /*get_URI(recv_buf, uri_buf);*/
                /*url_decode(uri_buf);*/
                /*printf("uri_buf %s\n",uri_buf);*/
                /*if(!strcmp(uri_buf, "/favicon.ico")){*/
                    /*//Server 主动关闭链接*/
                    /*close(fd);*/
                    /*FD_CLR(fd, &allset);*/
                    /*continue;*/
                /*}*/
                /*uri_status = get_URI_STATUS(uri_buf, rootcwd, currentcwd);*/
                /*printf("Status:%d\n", uri_status);*/
                /*switch (uri_status)*/
                /*{*/
                    /*case FILE_OK:*/
                        /*get_fileType(uri_buf, fileType);*/
                        /*send_file(fd, rootcwd, uri_buf, fileType);*/
                        /*close(fd);*/
                        /*FD_CLR(fd, &allset);*/
                        /*break;*/
                    /*case DIR_OK:*/
                        /*printDir(fd, rootcwd, uri_buf);*/
                        /*close(fd);*/
                        /*FD_CLR(fd, &allset);*/
                        /*break;*/
                    /*case FILE_NOT_FOUND:*/
                        /*if ((deteleFiles(fd, rootcwd, uri_buf)  != 0)&&*/
                                /*(getRenameInfo(fd, rootcwd, uri_buf)!= 0))*/
                        /*{*/
                            /*printf("file no found\n");*/
                        /*}*/
                        /*close(fd);*/
                        /*FD_CLR(fd, &allset);*/
                        /*break;*/
                    /*case FILE_FORBIDEN:*/
                        /*close(fd);*/
                        /*FD_CLR(fd, &allset);*/
                        /*break;*/
                    /*default:*/
                        /*break;*/
                /*}*/
            /*}*/
        /*}*/
        //Accept 结束

        /*调用select*/
        printf("Waiting Select...\n");
        res = select(MAX_SOCK_FD, &allset, NULL, NULL, NULL);
        switch (res)
        {
            case -1://select错误
                perror("Error in select!\n");
                close(listenfd);
                exit(-1);
                break;
            case 0://select超时
                printf("Waiting Client's Message!\n");
                continue;
                break;
            default:
                printf("Total %d res \n", res);
                for (fd = 0; fd < MAX_SOCK_FD; fd++)
                {
                    if (FD_ISSET(fd, &allset) > 0)
                    {
                        if (fd == listenfd) //处理客户端连接请求 listenfd
                        {
                            if ((clientfd = accept(listenfd, (struct sockaddr *)&client_sockaddr, &sin_size)) == -1)
                            {
                                perror("Error in Accept\n");
                                exit(1);
                            }
                            FD_SET(clientfd, &allset);
                            printf("New connection %d from %s\n",clientfd,inet_ntoa(client_sockaddr.sin_addr));
                        }
                        else //处理客户端消息 clientfd
                        {
                            if ((recvbytes = recv(fd, &recv_buf, BUFFER_SIZE, 0)) <= 0) //客户端关闭链接
                            {
                                close(fd);
                                FD_CLR(fd, &allset);
                                printf("Client %d has left\n",fd);
                            }
                            else //客户端发送消息
                            {
                                //[>打印请求报头<]
                                printf("\nHTTP header;\n %s \n", recv_buf);
                                if (!strstr(recv_buf, "HTTP/")) //非HTTP协议
                                {
                                    fprintf(stderr,"Not HTTP Protocol!\n");
                                    close(fd);
                                    FD_CLR(fd, &allset);
                                    exit(-1);
                                }
                                else
                                {
                                    get_URI(recv_buf, uri_buf);
                                    url_decode(uri_buf);
                                    printf("uri_buf %s\n",uri_buf);
                                    if(!strcmp(uri_buf, "/favicon.ico")){
                                        //[>[>Server 主动关闭链接<]<]
                                        close(fd);
                                        FD_CLR(fd, &allset);
                                        continue;
                                    }
                                    uri_status = get_URI_STATUS(uri_buf, rootcwd, currentcwd);
                                    printf("Status:%d\n", uri_status);
                                    switch (uri_status)
                                    {
                                        case FILE_OK:
                                            get_fileType(uri_buf, fileType);
                                            if( 0 == fork() ) //fork 子进程处理send
                                            {
                                                send_file(fd, rootcwd, uri_buf, fileType);
                                                exit(0);
                                            }
                                            close(fd);
                                            FD_CLR(fd, &allset);
                                            break;
                                        case DIR_OK:
                                         //   [>display_dir(fd, rootcwd, currentcwd, uri_buf);<]
                                            printDir(fd, rootcwd, uri_buf);
                                            close(fd);
                                            FD_CLR(fd, &allset);
                                            break;
                                        case FILE_NOT_FOUND:
                                            if ((deteleFiles(fd, rootcwd, uri_buf)  != 0)&&
                                                    (getRenameInfo(fd, rootcwd, uri_buf)!= 0))
                                            {
                                                printf("file no found\n");
                                            }
                                          //  [>display_error(fd, FILE_NOT_FOUND, uri_buf);<]
                                            close(fd);
                                            FD_CLR(fd, &allset);
                                            break;
                                        case FILE_FORBIDEN:
                                           // [>display_error(fd, FILE_FORBIDEN, uri_buf);<]
                                            close(fd);
                                            FD_CLR(fd, &allset);
                                            break;
                                        default:
                                            break;
                                    }
                                }
                            }
                        }
                    }

                }
        }
        /*select 结束*/
    }
    close(listenfd);
    exit(0);

}
