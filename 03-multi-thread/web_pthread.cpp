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
* 多线程版本的web server.
*/
void* handle(void* arg)
{
	Pthread_detach(pthread_self());
	int fd = (*(int *)arg);
	Free(arg);
	printf("%d: fd = %d\n", pthread_self(), fd);
	doit(fd);
	printf("%d: close fd = %d\n", pthread_self(), fd);
	close(fd);
}

int main(int argc, char *argv[])
{
	int listenfd = Open_listenfd(8080); /* 8080号端口监听 */
	signal(SIGPIPE, SIG_IGN); 
	int connfd;
	while (true) /* 无限循环 */
	{
		struct sockaddr_in clientaddr;
		socklen_t len = sizeof(clientaddr);
		
		int *fdp = (int *)Malloc(sizeof(int));
		*fdp = Accept(listenfd, (SA*)&clientaddr, &len);
		pthread_t tid;
		Pthread_create(&tid, NULL, handle, (void *)fdp);
		//close(connfd);
	}
	return 0;
}