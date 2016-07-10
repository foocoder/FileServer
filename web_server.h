// ---- Program Info Start----
//FileName:     web_server.h
//
//Author:       Fuchen Duan
//
//Email:        slow295185031@gmail.com
//
//CreatedAt:    2016-06-30 21:17:02
// ---- Program Info End  ----

#ifndef _WEB_SERVER_H_
#define _WEB_SERVER_H_

#define MAX_SOCK_FD FD_SETSIZE
#define PORT 8082
#define BUFFER_SIZE 1024
#define MAX_QUE_CONN_NM 5
#define MAX_LEN 1024
#define FILE_BUF_LEN  10240
#define MAX_NAME 255

#define FILE_OK 200
#define DIR_OK 100
#define FILE_NOT_FOUND 404
#define FILE_FORBIDEN 403

#define GET 0
#define POST 1

#define SEPCHARS " =\"\r\n"

struct sendFileArgs{
    int fd;
    int clientIndex;
    char rootcwd[BUFFER_SIZE];
    char uri_buf[MAX_NAME];
    char fileType[MAX_NAME];
    long long fileSize;
};

struct uploadArgs{
    int sockfd;
    int clientIndex;
    int recvbytes;
    char header_buf[BUFFER_SIZE*2];
    char rootcwd[BUFFER_SIZE];
    char uri_buf[MAX_NAME];
};

struct RequestLine{
    int method;
    char uri[MAX_NAME];
    int version;
};

struct UploadFileInfo{
    long long contentLength;
    char contentType[MAX_NAME];
    char boundary[MAX_NAME];
};

struct BodyInfo{
    char filename[MAX_NAME];
};
void get_URI(char * recv_buf, char * uri_buf);
void url_decode(char *pURL);
int get_URI_STATUS(char * uri_buf, char * rootcwd, char * currentcwd);
void get_fileType(char *filename, char *fileType);
long send_file(int fd, char * rootcwd, char * uri_buf, char * fileType);
int display_error(int fd, int error_no, char * uri_buf);
void waitingForClientSelectMax(int listenfd);
void waitingForClientSelectSimple(int listenfd);
void * threadSendFile(void * arg);
void * threadUpload(void * arg);
struct RequestLine process_request_line(char * recv_buf);
struct UploadFileInfo process_request_head( char * header_buf );
struct BodyInfo get_fileName( char * body_header );

#endif
