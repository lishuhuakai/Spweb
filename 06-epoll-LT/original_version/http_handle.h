#ifndef _HTTP_HANDLE_H_
#define _HTTP_HANDLE_H_

#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>
#include "csapp.h"
#include "epoll_ulti.h"
#include "noncopyable.h"
#include "state.h"

/*
* 这个类主要是用于处理http请求的!
*/
class http_handle : public noncopyable /* 不可以被拷贝,不可以被复制 */
{
public:
	static const int READ_BUFFER_SIZE = 8192; /* 读缓冲区的大小 */
	static const int WRITE_BUFFER_SIZE = 89400; /* 写缓冲区的大小 */
public:
	void init(int fd);
	ResState processRead();
	ResState processWrite();
	
private:
	bool addResponse(const char* format, ...);
	bool addResponse(char * const);
	bool copyData(void* src, int);
	int getLine(char *buf, int maxsize);
	void resetData();
	void static getFileType(char *filename, char *filetype);
	void clienterror(char *cause, char *errnum, char *shortmsg, char *longmsg);
	void serveStatic(char *filename);
	void serveDynamic(char *filename, char *cgiargs);
	void readRequestHdrs(); /* 读取请求头 */
	int parseUri(char *uri, char *filename, char *cgiargs);
	bool read();

private:
	/* 该HTTP连接的socket和对方的socket地址 */
	int sockfd_;
	char* fileAddress_; /* 文件的地址 */
	int fileSize_; /* 文件的大小 */

	/* 读缓冲区 */
	char readBuf_[READ_BUFFER_SIZE];
	/* 标志读缓冲区中已经读入的客户数据的最后一个字节的下一个位置 */
	int nRead_;
	/* 当前正在分析的字符在读缓冲区中的位置 */
	int nChecked_;

	bool keepAlive_;
	int written_; /* 已经写了多少字节 */

	/* 写缓冲区 */
	char writeBuf_[WRITE_BUFFER_SIZE];

	/* 写缓冲区中待发送的字节数 */
	int nStored_;
};
#endif
