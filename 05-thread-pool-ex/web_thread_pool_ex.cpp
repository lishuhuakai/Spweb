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
#include "condition.h"

/*-
* 线程池的加强版本.主要是主线程统一接收连接,其余都是工作者线程,这里的布局非常类似于一个生产者.
* 多个消费者.
*/
/* 网页的根目录 */
const char * root_dir = "/home/lishuhuakai/WebSiteSrc/html_book_20150808/reference";
/* / 所指代的网页 */
const char * home_page = "index.html";

#define MAXNCLI 100

MutexLock mutex;
Condition cond(mutex);
int clifd[MAXNCLI], iget, iput;

int main(int argc, char *argv[])
{
	int listenfd = Open_listenfd(8080); /* 8080号端口监听 */
	signal(SIGPIPE, SIG_IGN);
	pthread_t tids[10];
	void* thread_main(void *);

	for (int i = 0; i < 10; ++i) {
		int *arg = (int *)Malloc(sizeof(int));
		*arg = i;
		Pthread_create(&tids[i], NULL, thread_main, (void *)arg);
	}
	struct sockaddr cliaddr; /* 用于存储对方的ip信息 */
	socklen_t clilen;
	for (; ; ) {
		int connfd = Accept(listenfd, &cliaddr, &clilen);
		{
			MutexLockGuard lock(mutex); /* 加锁 */
			clifd[iput] = connfd;
			if (++iput == MAXNCLI) iput = 0;
			if (iput == iget) unix_error("clifd is not big enough!\n");
		}
		cond.notify(); /* 通知一个线程有数据啦! */
	}
	return 0;
}

void*
thread_main(void *arg)
{
	int connfd;
	printf("thread %d starting\n", *(int *)arg);
	Free(arg);
	for ( ; ;) {
		{
			MutexLockGuard lock(mutex); /* 加锁 */
			while (iget == iput) { /* 没有新的连接到来 */
				/*-
				* 代码必须用while循环来等待条件变量,原因是spurious wakeup
				*/
				cond.wait(); /* 这一步会原子地unlock mutex并进入等待,wait执行完毕会自动重新加锁 */
			}
			connfd = clifd[iget]; /* 获得连接套接字 */
			if (++iget == MAXNCLI) iget = 0;
		}
		doit(connfd);
		close(connfd);
	}
}
