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
#include "http_handle.h"
#include "state.h"

/*-
* 非阻塞版本的web server,主要利用epoll机制来实现多路IO复用.
* 将阻塞版本变为非阻塞版本之后,会增加复杂性,稍微举一个例子,因为文件描述符变得非阻塞了,往文件描述符里面写入数据的时候,
* 如果tcp缓存区已满,那么我们暂时还不能往里面写入数据,那么write函数会立马返回,而不是像以前那样阻塞在那里,一直等到缓冲区
* 可用.这样以来的话,由于一次性写不完,我们最好还要自己弄一个buf,还要记录已经写了多少数据,同时因为没有写完,所以还不能关闭
* 连接,要一直到写完才能关闭连接.这样以来程序的复杂度大大提高.
*/

/* 网页的根目录 */
const char * root_dir = "/home/lishuhuakai/WebSiteSrc/html_book_20150808/reference";
/* / 所指代的网页 */
const char * home_page = "index.html";
#define MAXEVENTNUM 100

void addsig(int sig, void(handler)(int), bool restart = true)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = handler;
	if (restart) {
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

int main(int argc, char *argv[])
{
	int listenfd = Open_listenfd(8080); /* 8080号端口监听 */
	epoll_event events[MAXEVENTNUM];
	sockaddr clnaddr;
	socklen_t clnlen = sizeof(clnaddr);

	addsig(SIGPIPE, SIG_IGN);
	
	int epollfd = Epoll_create(80); /* 10基本上没有什么用处 */
	addfd(epollfd, listenfd, false); /* epollfd要监听listenfd上的可读事件 */
	
	HttpHandle handle[256];
	int acnt = 0;
	for ( ; ;) {
		int eventnum = Epoll_wait(epollfd, events, MAXEVENTNUM, -1);
		for (int i = 0; i < eventnum; ++i) {
			int sockfd = events[i].data.fd;
			if (sockfd == listenfd) { /* 有连接到来 */
				printf("%d\n", ++acnt);
				int connfd = Accept(listenfd, &clnaddr, &clnlen);
				handle[connfd].init(connfd); /* 初始化 */
				addfd(epollfd, connfd, false); /* 加入监听 */
			}
			else if (events[i].events & EPOLLIN) { /* 有数据可读 */
				int res = handle[sockfd].processRead(); /* 处理读事件 */
				if (res == STATUS_WRITE)  /* 我们需要监听写事件 */
					modfd(epollfd, sockfd, EPOLLOUT);
				else 
					removefd(epollfd, sockfd);
			}
			else if (events[i].events & EPOLLOUT) { /* 如果可写了 */
				printf("Could write!\n");
				int res = handle[sockfd].processWrite(); /* 处理写事件 */
				if (res == STATUS_READ) /* 对方发送了keepalive */
					modfd(epollfd, sockfd, EPOLLIN);
				else
					removefd(epollfd, sockfd);
			}
		}
	}
	return 0;
}

