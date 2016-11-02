#ifndef _EPOLL_ULTI_H_
#define _EPOLL_ULTI_H_

#include <unistd.h>
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

namespace utility 
{

#define mylog(formatPM, args...)\
  mlog(pthread_self(), __FILE__, __LINE__, __FUNCTION__,  (formatPM) , ##args)

	void mlog(pthread_t tid, const char *fileName, int lineNum, const char *func, const char *log_str, ...);
	void removefd(int epollfd, int fd);
	void modfd(int epollfd, int fd, int ev,bool oneShot);
	void addfd(int epollfd, int fd, bool one_shot);
	int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
	int Epoll_create(int size);

}

#endif
