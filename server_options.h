//////////////////////////////////////////////////
//
//	server_options.h
//	@Pan Hu	12/29/2014
//
//////////////////////////////////////////////////

#ifndef _SERVER_OPTIONS_H_
#define _SERVER_OPTIONS_H_ 

#define	DIR_LENGTH		256		/* length of directory */
#define MAX_BUFFER_SIZE	102400	/* buffer size of message head and message txt */

int getParentDir (char parent_dir[], char sub_dir[]);
int printDir 	 (int fd, char web_root[], char offset_dir[]);
int deteleFiles	 (int fd, char web_root[], char offset_dir[]);
int getRenameInfo(int fd, char web_root[], char url_link[]);
int reName		 (char path[], char name[]);

#endif