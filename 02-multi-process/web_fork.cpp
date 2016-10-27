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

/* 网页的根目录 */
const char * root_dir = "/home/lishuhuakai/WebSiteSrc/html_book_20150808/reference";
/* / 所指代的网页 */
const char * home_page = "index.html";

/*- 
* 多进程版本的web server,同时也是最简单的一个并发服务器.比单进程版本的server效率有所提高.
* 但是,connfd仍然是阻塞的,也就是说,对这些个套接字read,write的时候必须要等到有数据了才行,
* 效率仍然不高.
*/
void addsig(int sig, void(handler)(int), bool restart = true)
{
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if (restart)
	{
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, NULL) != -1);
}

void
sig_chld(int signo) /* 处理僵死进程 */
{
	pid_t pid;
	int stat;
	while ((pid = waitpid(-1, &stat, WNOHANG)) > 0)
		printf("child %d terminated\n", pid);
	return;
}

int main(int argc, char *argv[])
{
	int listenfd = Open_listenfd(8080); /* 8080号端口监听 */

	while (true) /* 无限循环 */
	{
		struct sockaddr_in clientaddr;
		socklen_t len = sizeof(clientaddr);
		int connfd = Accept(listenfd, (SA*)&clientaddr, &len);
		addsig(SIGCHLD, sig_chld); /* 添加信号处理函数 */
		int pid;
		if ((pid = Fork()) == 0) /* 子线程 */
		{
			printf("\nnew process:%d\n", getpid());
			close(listenfd);
			doit(connfd);
			close(connfd);
			printf("\nend of process:%d\n", getpid());
			exit(0);
		}
		close(connfd);
	}
	return 0;
}