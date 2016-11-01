#ifndef _HTTP_HANDLE_H_
#define _HTTP_HANDLE_H_

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <sys/stat.h>
#include <string.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "csapp.h"
#include "noncopyable.h"
#include "state.h"
#include "cache.h"
#include "epoll_ulti.h"
#include "mutex.h"

/*
* HttpHandle类主要是用于处理http请求的,更具体来说,是为了处理静态网页.
*/
/* 这里默认是私有继承 */
class HttpHandle : noncopyable /* 不可以被拷贝,不可以被复制 */
{
public:
	void init(int fd);
	void process();
	static void setEpollfd(int epollfd) { epollfd_ = epollfd; }
private:
	enum States{kRead, kWrite, kOver, kError};
	void setState(States s) { state_ = s; }
	States state_;
private:
	int processRead();
	int processWrite();
	void reset();
	bool addResponse(const char* format, ...);
	bool addResponse(char * const);
	int getLine(char *buf, int maxsize);
	void static getFileType(char *fileName, char *fileType);
	void clientError(char *cause, char *errnum, char *shortmsg, char *longmsg);
	void serveStatic(char *filename, size_t fileSize);
	void readRequestHdrs(); /* 读取请求头 */
	int parseUri(char *uri, char *fileName, char *cgiargs);
	bool read();
private:
	static const int READ_BUFFER_SIZE = 1024; /* 读缓冲区的大小 */
	static const int WRITE_BUFFER_SIZE = 1024; /* 写缓冲区的大小 */
	static const char rootDir_[]; /* 网页的根目录 */
	static const char homePage_[]; /* 所指代的网页 */
	static Cache& cache_; /* 全局只需要一个cache_*/
	static int epollfd_;
private:
	/* 该HTTP连接的socket和对方的socket地址 */
	int sockfd_;
	boost::shared_ptr<FileInfo> fileInfo_;

	/* 读缓冲区 */
	char readBuf_[READ_BUFFER_SIZE];
	/* 标志读缓冲区中已经读入的客户数据的最后一个字节的下一个位置 */
	size_t nRead_;
	/* 当前正在分析的字符在读缓冲区中的位置 */
	size_t nChecked_;

	bool keepAlive_; /* 是否保持连接 */
	bool sendFile_; /* 是否发送文件 */
	
	/* 写缓冲区 */
	char writeBuf_[WRITE_BUFFER_SIZE];
	/* 写缓冲区中待发送的字节数 */
	size_t nStored_;
	/* 已经写了多少字节 */
	size_t written_; 
};
#endif
