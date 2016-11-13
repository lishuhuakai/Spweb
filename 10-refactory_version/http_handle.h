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
#include <memory>
#include "csapp.h"
#include "noncopyable.h"
/// #include "cache.h"
#include "epoll_ulti.h"
#include "mutex.h"
#include "http_request.h"
#include "buff.h"
#include "common.h"
using namespace utility;

/*
* HttpHandle类主要是用于处理http请求的,更具体来说,是为了处理静态网页.
*/

/* 这里默认是私有继承 */
class Cache; /* 前向声明 */
class FileInfo;
class HttpHandle : noncopyable /* 不可以被拷贝,不可以被复制 */
{
public:
	void init(int fd);
	void process();
	static void setEpollfd(int epollfd) { epollfd_ = epollfd; }
public:
	enum HttpHandleState {
		kExpectReset, /* 需要初始化 */
		kExpectRead, /* 正在处理读 */
		kExpectWrite, /* 正在处理写 */
		kError, /* 出错 */
		kSuccess, /* 成功 */
		kClosed /* 对方关闭连接 */
	};
private:
	HttpHandleState state_;
	HttpRequest request_;
private:
	void processRead();
	void processWrite();
	void reset();
	void clientError(const char *, const char *, const char *, const char *);
	void serveStatic(const char *filename, size_t fileSize);
	bool read();
private:
	
	static Cache& cache_; /* 全局只需要一个cache_*/
	static int epollfd_;
private:
	/* 该HTTP连接的socket和对方的socket地址 */
	int sockfd_;
	pInfo fileInfo_;

	/* 读缓冲区 */
	Buffer readBuffer_;

	bool keepAlive_; /* 是否保持连接 */
	bool sendFile_; /* 是否发送文件 */
	
	/* 写缓冲区 */
	Buffer writeBuffer_;
	/* 已经写了多少字节 */
	size_t fileWritten_; /* 文件写了多少字节 */ 
};
#endif
