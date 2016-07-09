//////////////////////////////////////////////////
//
//	dir.h
//	@Pan Hu	12/29/2014
//
//////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <time.h>
#include "server_options.h"

/*
 *
 */
int getParentDir (char parent_dir[], char sub_dir[])
{
    int len = strlen(sub_dir);

    int i,j;
    for (i = len-1; i >= 0; i--)
    {
        if ( ( *(sub_dir+i) == '/') )
        {
            if ( i == 0 )
            {
                *(parent_dir+0) = '/';
                break;
            }
            if ( i != len-1 )
            {
                for (j = 0; j < i; j++)
                {
                    *(parent_dir+j) = *(sub_dir+j);
                }
                break;
            }
        }//if (i =len-1
    }//for
    *(parent_dir+strlen(parent_dir)) = '\0';
    //	printf("getParentDir %s\n", parent_dir);
    return 0;
}

/*
 *
 */
int printDir (int fd, char web_root[], char offset_dir[])
{
    DIR *dir;
    struct dirent *dirp;
    struct stat statbuf;

    char buf[DIR_LENGTH];
    int num_dir,num_reg;
    char msg_head[MAX_BUFFER_SIZE], msg_txt[MAX_BUFFER_SIZE];
    memset(buf,0,DIR_LENGTH*sizeof(char));
    memset(msg_txt,0,MAX_BUFFER_SIZE*sizeof(char));
    memset(msg_head,0,MAX_BUFFER_SIZE*sizeof(char));
    //head
    sprintf(msg_head, "HTTP/1.1 WEB SERVER\r\n");
    sprintf(msg_head, "%sServer: Web Server\r\n",msg_head);
    sprintf(msg_head, "%sContent-Type:text/html\r\n",msg_head);
    //txt
    sprintf(msg_txt, "<html><meta charset=\"UTF-8\">");
    sprintf(msg_txt, "%s<head><title>WEB SERVER</title></head>",msg_txt);
    sprintf(msg_txt, "%s<body><h1>WEB SERVER</h1>",msg_txt);
    if ( strcmp(offset_dir,"/") != 0 )
    {
        getParentDir(buf,offset_dir);
        printf("buf = %s\n", buf);
        sprintf(msg_txt,"%s<p><a href=\"/\">根目录: /</a>&#160&#160<a href=\"%s\">父目录: %s</a></p>",
                msg_txt,buf,buf);
    }
    sprintf(msg_txt, "%s<p>----当前目录: %s</p>",msg_txt,offset_dir);
    sprintf(msg_txt,"%s<form action=\"rename.%s\" method=\"get\">", msg_txt,offset_dir);
    int len = strlen(offset_dir);
    if ( ( *(offset_dir+len-1) == '/') )
    {
        if (len == 1)
        {
            offset_dir = "\0";
        }
        else
        {
            *(offset_dir+len-1) = '\0';
        }
    }

    char  path[DIR_LENGTH];
    memset(path, 0, DIR_LENGTH*sizeof(char));
    strcpy(path,web_root);
    strcat(path,offset_dir);
    printf("当前磁盘目录: %s\n", path);
    printf("在服务器器中的偏移目录%s\n", offset_dir);

    //open dir
    if ( (dir = opendir(path))  ==  NULL )
    {
        printf("can't open %s", path);
        exit(1);
        //		sprintf(msg_txt, "%s<p>can't open %s</p>",msg_txt,path);
    }

    //tranvers dir
    num_dir = 0;
    num_reg = 0;
    //				   <form action="/html" method="get">
    //sprintf(msg_txt,"%s<form action=\"rename\" method=\"get\">", msg_txt);
    while ( (dirp = readdir(dir)) != NULL)
    {
        sprintf(buf,"%s/%s",path,dirp->d_name);
        if (stat(buf, &statbuf) == -1)
        {
            printf("Get stat on %s\n", buf);
            exit(1);
            //        	sprintf(msg_txt, "%s<p>can't get stat on %s</p>",msg_txt,path);
        }
        if ( S_ISREG(statbuf.st_mode) )
        {
            printf("%s\t\t%ld字节\t\t%s\n",dirp->d_name, statbuf.st_size, ctime(&statbuf.st_mtime));
            sprintf(msg_txt,"%s<p><a href=\"%s/%s\">%s</a>&#160&#160&#160&#160 %ld -字节&#160&#160%s",
                    msg_txt,offset_dir,dirp->d_name,dirp->d_name,statbuf.st_size,ctime(&statbuf.st_mtime));
            sprintf(msg_txt,"%s&#160&#160<a href=\"%s/%s\">下载</a>&#160&#160<a href=\"%s/%s.\">删除</a>",
                    msg_txt,offset_dir,dirp->d_name,offset_dir,dirp->d_name);
            sprintf(msg_txt,"%s&#160&#160<input type=\"text\"name=\"%s/%s\"style=\"width:50px\"/>",
                    msg_txt,offset_dir,dirp->d_name);
            sprintf(msg_txt,"%s<input type=\"submit\"value=\"更名\"style=\"width:50px\"/></p>",
                    msg_txt);//style=\"height:22px;width:50px\"
            num_reg++;
        }
        else
        {
            if ( strcmp(dirp->d_name,".")  && strcmp(dirp->d_name,"..") )
            {
                printf("%s\t\t文件夹\t\t%s\n",dirp->d_name,ctime(&statbuf.st_mtime));
                sprintf(msg_txt,"%s<p><a href=\"%s/%s\">%s</a>&#160&#160文件夹&#160&#160%s",
                        msg_txt,offset_dir,dirp->d_name,dirp->d_name,ctime(&statbuf.st_mtime));
                sprintf(msg_txt,"%s&#160&#160<a href=\"%s/%s\">打开</a>&#160&#160<a href= \"%s/%s.\">删除</a>",
                        msg_txt,offset_dir,dirp->d_name,offset_dir,dirp->d_name);
                sprintf(msg_txt,"%s&#160&#160<input type=\"text\"name=\"%s/%s\"style=\"width:50px\"/>",
                        msg_txt,offset_dir,dirp->d_name);
                sprintf(msg_txt,"%s<input type=\"submit\"value=\"更名\"style=\"width:50px\"/></p>",
                        msg_txt);
                num_dir++;
            }
        }
    }//while
    sprintf(msg_txt,"%s</form>",msg_txt);
    //close dir
    closedir(dir);
    printf("路径\"%s\"下有%d个目录，%d个文件!\n", path, num_dir, num_reg);
    sprintf(msg_txt, "%s<p>当前路径下有%d个目录，%d个文件!</p>",msg_txt,num_dir,num_reg);
    sprintf(msg_txt, "%s<form enctype=\"multipart/form-data\" action=\"/\" method=post>\n", msg_txt);
    sprintf(msg_txt, "%s<input name=\"uploadFile\" type=\"file\"><br> \n", msg_txt);
    sprintf(msg_txt, "%s<input type=\"submit\" value=\"提交\"><input type=reset></form> </body></html>", msg_txt);
    sprintf(msg_head,"%sContent-Length:%d\r\n\r\n",msg_head,(int) strlen(msg_txt));
    if (send(fd, msg_head, strlen(msg_head), 0) == -1)
    {
        perror("Send Head Fail --printDir\n");
        exit(1);
    }
    if (send(fd, msg_txt, strlen(msg_txt), 0) == -1)
    {
        perror("Send Txt Fail --printDir\n");
        exit(1);
    }
    return 0;
}

/*
 *
 */
int deteleFiles (int fd, char web_root[], char offset_dir[])
{
    int len = strlen(offset_dir);
    if ( ( *(offset_dir+len-1) != '.') )
    {
        printf("_dir%s\n", offset_dir);
        printf("The last character is not '.',not delete command\n");
        return -1;
    }
    *(offset_dir+len-1) = '\0';

    char path[DIR_LENGTH];
    memset(path,0,DIR_LENGTH*sizeof(char));
    strcpy(path,web_root);
    strcat(path,offset_dir);
    printf("delete file %s\n",path);

    char parent_dir[DIR_LENGTH] ;
    memset(parent_dir,0,DIR_LENGTH*sizeof(char));
    getParentDir(parent_dir,offset_dir);
    printf("uri %s  parent dir %s\n",offset_dir,parent_dir);

    if( remove(path) != 0 )
    {
        perror("delete file error!");
        return -1;
    }
    printDir(fd, web_root, parent_dir);
    return 0;
}
// /?/web_server.c=111&/1.html=&/2.html=&/server_options.c=&/web=&/dir.c~=&/server_options.h=&/no=
int getRenameInfo (int fd, char web_root[], char url_link[])
{
    char file_tmp[DIR_LENGTH];
    memset(file_tmp, 0, DIR_LENGTH*sizeof(char));
    char name_tmp[32];
    memset(name_tmp, 0, 32*sizeof(char));
    char path_tmp[DIR_LENGTH];
    memset(path_tmp, 0, DIR_LENGTH*sizeof(char));
    char current_path[DIR_LENGTH];
    memset(current_path, 0, DIR_LENGTH*sizeof(char));

    int i, j;
    int pos_equal = 0;
    int pos_and   = 0;

    sscanf(url_link, "%*[^.].%[^?]", current_path);
    printf("current path1 %s\n", current_path);
    char* seps = ".";
    char* token = strtok( current_path, seps );
    while( token != NULL )
    {
        strcpy(current_path,token);
        token = strtok( NULL, seps );
    }
    //	memset(current_path, 0, DIR_LENGTH*sizeof(char));

    printf("current path2 %s\n", current_path);

    char strTmp[DIR_LENGTH];
    strcpy(strTmp,url_link);
    seps = "/&";
    token = strtok( strTmp, seps );
    while( token != NULL )
    {
        /* While there are tokens in "string" */
        printf( " %s\n", token );

        /* process:get new name */
        sscanf(token, "%[^=]=%s", file_tmp, name_tmp);	//get file and new name
        if (strlen(name_tmp) != 0)
        {
            printf("file %s\nname %s  len= %d\n", file_tmp, name_tmp,strlen(name_tmp));
            sprintf(path_tmp,"%s%s/%s",web_root,current_path,file_tmp);
            printf("file %s\n", path_tmp);

            if ( reName(path_tmp,name_tmp) == 0 )
            {
                printf("reName%s to  %s\n",path_tmp,name_tmp );
            }

            memset(name_tmp, 0, 32*sizeof(char));
            memset(path_tmp, 0, DIR_LENGTH*sizeof(char));
            memset(file_tmp, 0, DIR_LENGTH*sizeof(char));
        }
        /* Get next token: */
        token = strtok( NULL, seps );
    }
    printDir(fd, web_root, current_path);
    return 0;
}
int reName(char path[], char name[])
{
    if ( rename(path,name) == 0 )
    {
        return 0;
    }
    return -1;
}
