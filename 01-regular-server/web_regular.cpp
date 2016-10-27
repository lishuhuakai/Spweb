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
* 单进程版本的web server!当没有连接到来的时候,该进程会阻塞在Accept函数上,当然,这里的connfd也是阻塞版本的.
* 也就是说,在对connfd执行write,read等函数的时候也会阻塞.这样一来,这个版本的server效率会非常低下.
*/
int main(int argc, char *argv[])
{
	int listenfd = Open_listenfd(8080); /* 8080号端口监听 */
							
	while (true) /* 无限循环 */
	{
		struct sockaddr_in clientaddr;
		socklen_t len = sizeof(clientaddr);
		int connfd = Accept(listenfd, (SA*)&clientaddr, &len);
		doit(connfd);
		Close(connfd);
	}
	return 0;
}