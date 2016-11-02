#include "epoll_ulti.h"

namespace utility
{
	void mlog(pthread_t tid, const char *fileName, int lineNum, const char *func, const char *log_str, ...)
	{
		va_list vArgList; //定义一个va_list型的变量,这个变量是指向参数的指针.
		char buf[1024];
		va_start(vArgList, log_str); //用va_start宏初始化变量,这个宏的第二个参数是第一个可变参数的前一个参数,是一个固定的参数
		vsnprintf(buf, 1024, log_str, vArgList); //注意,不要漏掉前面的_
		va_end(vArgList);  //用va_end宏结束可变参数的获取
		printf("%lu:%s:%d:%s --> %s\n", tid, fileName, lineNum, func, buf);
	}

	int setnonblocking(int fd) /* 将文件描述符设置为非阻塞 */
	{
		int old_option = fcntl(fd, F_GETFL);
		int new_option = old_option | O_NONBLOCK;
		fcntl(fd, F_SETFL, new_option);
		return old_option;
	}

	void addfd(int epollfd, int fd, bool one_shot)
	{
		epoll_event event;
		event.data.fd = fd;
		event.events = EPOLLIN | EPOLLET; /* ET触发 */
		if (one_shot) {
			event.events |= EPOLLONESHOT; /* 这里特别要注意一下 */
		}
		epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
		setnonblocking(fd);
	}

	void removefd(int epollfd, int fd)
	{
		//mylog("removefd()\n");
		epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, 0);
		close(fd);
	}

	void modfd(int epollfd, int fd, int ev, bool oneShot)
	{
		epoll_event event;
		event.data.fd = fd;
		event.events = ev | EPOLLET; /* ET触发 */
		if (oneShot) {
			event.events |= EPOLLONESHOT;
		}
		epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
	}

	int Epoll_create(int size)
	{
		int rc;
		if ((rc = epoll_create(size)) < 0) {
			unix_error("epoll_crate failed");
		}
		return rc; /* 否则的话,返回文件描述符 */
	}

	int Epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout)
	{
		int rc;
		if ((rc = epoll_wait(epfd, events, maxevents, timeout)) == -1) {
			unix_error("epoll failed");
		}
		return rc;
	}
}
