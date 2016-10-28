#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <cassert>
#include <sys/epoll.h>
#include "csapp.h"
#include "epoll_ulti.h"
#include "http_server.h"
#include "mutex.h"

/* 网页的根目录 */
const char * root_dir = "/home/lishuhuakai/WebSiteSrc/html_book_20150808/reference";
/* / 所指代的网页 */
const char * home_page = "index.html";


/*- 
* 线程池版本的web server.主要的思想是事先构建一个线程池,只是需要注意的是,accept的时候需要加锁.
*/

int listenfd; /* 全局的一个监听套接字 */
MutexLock mutex;

int main(int argc, char *argv[])
{
	listenfd = Open_listenfd(8080); /* 8080号端口监听 */
	//signal(SIGPIPE, SIG_IGN); 

	pthread_t tids[10];
	void* thread_main(void *);

	for (int i = 0; i < 10; ++i) {
		int *arg = (int *)Malloc(sizeof(int));
		*arg = i;
		Pthread_create(&tids[i], NULL, thread_main, (void *)arg);
	}
	for ( ; ; )
		pause();
	return 0;
}

void* thread_main(void *arg)
{
	printf("thread %d starting\n", *(int*)arg);
	Free(arg);
	struct sockaddr cliaddr;
	socklen_t clilen;
	int connfd;
	while (true) {
		{
			MutexLockGuard lock(mutex); /* 加锁 */
			connfd = Accept(listenfd, &cliaddr, &clilen);
		}
		doit(connfd);
		close(connfd);
	}
}