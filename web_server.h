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

#define FILE_OK 200
#define DIR_OK 100
#define FILE_NOT_FOUND 404
#define FILE_FORBIDEN 403

#define GET 0
#define POST 1

#define SEPCHARS " =\r\n"

struct sendFileArgs{
    int fd;
    char * rootcwd;
    char * uri_buf;
    char * fileType;
    long long filesize;
};

struct RequestLine{
    int method;
    char * uri;
    int version;
};

struct UploadFileInfo{
    long contentLength;
    char * contentType;
    char * boundary;
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
struct RequestLine process_request_line(char * recv_buf);
struct UploadFileInfo process_request_head();

#endif
