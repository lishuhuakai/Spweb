/* $begin csapp.c */
#include "csapp.h"

namespace utility
{

	/**************************
	* Error-handling functions
	**************************/
	/* $begin errorfuns */
	/* $begin unixerror */
	void unix_error(const char *msg) /* unix-style error */
	{
		fprintf(stderr, "%s: %s\n", msg, strerror(errno));
		exit(0);
	}
	/* $end unixerror */

	void posix_error(int code, const char *msg) /* posix-style error */
	{
		fprintf(stderr, "%s: %s\n", msg, strerror(code));
		exit(0);
	}

	void dns_error(const char *msg) /* dns-style error */
	{
		fprintf(stderr, "%s: DNS error %d\n", msg, h_errno);
		exit(0);
	}

	void app_error(const char *msg) /* application error */
	{
		fprintf(stderr, "%s\n", msg);
		exit(0);
	}
	/* $end errorfuns */

	/*********************************************
	* Wrappers for Unix process control functions
	********************************************/

	/* $begin forkwrapper */
	pid_t Fork(void)
	{
		pid_t pid;

		if ((pid = fork()) < 0)
			unix_error("Fork error");
		return pid;
	}
	/* $end forkwrapper */

	void Execve(const char *filename, char *const argv[], char *const envp[])
	{
		if (execve(filename, argv, envp) < 0)
			unix_error("Execve error");
	}

	/* $begin wait */
	pid_t Wait(int *status)
	{
		pid_t pid;

		if ((pid = wait(status)) < 0)
			unix_error("Wait error");
		return pid;
	}
	/* $end wait */

	pid_t Waitpid(pid_t pid, int *iptr, int options)
	{
		pid_t retpid;

		if ((retpid = waitpid(pid, iptr, options)) < 0)
			unix_error("Waitpid error");
		return(retpid);
	}

	/* $begin kill */
	void Kill(pid_t pid, int signum)
	{
		int rc;

		if ((rc = kill(pid, signum)) < 0)
			unix_error("Kill error");
	}
	/* $end kill */

	void Pause()
	{
		(void)pause();
		return;
	}

	unsigned int Sleep(unsigned int secs)
	{
		unsigned int rc;

		if ((rc = sleep(secs)) < 0)
			unix_error("Sleep error");
		return rc;
	}

	unsigned int Alarm(unsigned int seconds) {
		return alarm(seconds);
	}

	void Setpgid(pid_t pid, pid_t pgid) {
		int rc;

		if ((rc = setpgid(pid, pgid)) < 0)
			unix_error("Setpgid error");
		return;
	}

	pid_t Getpgrp(void) {
		return getpgrp();
	}

	/************************************
	* Wrappers for Unix signal functions
	***********************************/

	/* $begin sigaction */
	handler_t *Signal(int signum, handler_t *handler) /* 用于注册信号处理函数 */
	{
		struct sigaction action, old_action;

		action.sa_handler = handler;
		sigemptyset(&action.sa_mask); /* block sigs of type being handled */
		action.sa_flags = SA_RESTART; /* restart syscalls if possible */

		if (sigaction(signum, &action, &old_action) < 0)
			unix_error("Signal error");
		return (old_action.sa_handler);
	}
	/* $end sigaction */

	void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
	{
		if (sigprocmask(how, set, oldset) < 0)
			unix_error("Sigprocmask error");
		return;
	}

	void Sigemptyset(sigset_t *set)
	{
		if (sigemptyset(set) < 0)
			unix_error("Sigemptyset error");
		return;
	}

	void Sigfillset(sigset_t *set)
	{
		if (sigfillset(set) < 0)
			unix_error("Sigfillset error");
		return;
	}

	void Sigaddset(sigset_t *set, int signum)
	{
		if (sigaddset(set, signum) < 0)
			unix_error("Sigaddset error");
		return;
	}

	void Sigdelset(sigset_t *set, int signum)
	{
		if (sigdelset(set, signum) < 0)
			unix_error("Sigdelset error");
		return;
	}

	int Sigismember(const sigset_t *set, int signum)
	{
		int rc;
		if ((rc = sigismember(set, signum)) < 0)
			unix_error("Sigismember error");
		return rc;
	}


	/********************************
	* Wrappers for Unix I/O routines
	********************************/

	int Open(const char *pathname, int flags, mode_t mode)
	{
		int rc;

		if ((rc = open(pathname, flags, mode)) < 0)
			unix_error("Open error");
		return rc;
	}

	ssize_t Read(int fd, void *buf, size_t count)
	{
		ssize_t rc;

		if ((rc = read(fd, buf, count)) < 0)
			unix_error("Read error");
		return rc;
	}

	ssize_t Write(int fd, const void *buf, size_t count)
	{
		ssize_t rc;

		if ((rc = write(fd, buf, count)) < 0)
			unix_error("Write error");
		return rc;
	}

	off_t Lseek(int fildes, off_t offset, int whence)
	{
		off_t rc;

		if ((rc = lseek(fildes, offset, whence)) < 0)
			unix_error("Lseek error");
		return rc;
	}

	void Close(int fd)
	{
		int rc;

		if ((rc = close(fd)) < 0)
			unix_error("Close error");
	}

	int Select(int  n, fd_set *readfds, fd_set *writefds,
		fd_set *exceptfds, struct timeval *timeout)
	{
		int rc;

		if ((rc = select(n, readfds, writefds, exceptfds, timeout)) < 0)
			unix_error("Select error");
		return rc;
	}

	int Dup2(int fd1, int fd2)
	{
		int rc;

		if ((rc = dup2(fd1, fd2)) < 0)
			unix_error("Dup2 error");
		return rc;
	}

	void Stat(const char *filename, struct stat *buf)
	{
		if (stat(filename, buf) < 0)
			unix_error("Stat error");
	}

	void Fstat(int fd, struct stat *buf)
	{
		if (fstat(fd, buf) < 0)
			unix_error("Fstat error");
	}

	/***************************************
	* Wrappers for memory mapping functions
	***************************************/
	void *Mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
	{
		void *ptr;

		if ((ptr = mmap(addr, len, prot, flags, fd, offset)) == ((void *)-1))
			unix_error("mmap error");
		return(ptr);
	}

	void Munmap(void *start, size_t length)
	{
		if (munmap(start, length) < 0)
			unix_error("munmap error");
	}

	/***************************************************
	* Wrappers for dynamic storage allocation functions
	***************************************************/

	void *Malloc(size_t size)
	{
		void *p;

		if ((p = malloc(size)) == NULL)
			unix_error("Malloc error");
		return p;
	}

	void *Realloc(void *ptr, size_t size)
	{
		void *p;

		if ((p = realloc(ptr, size)) == NULL)
			unix_error("Realloc error");
		return p;
	}

	void *Calloc(size_t nmemb, size_t size)
	{
		void *p;

		if ((p = calloc(nmemb, size)) == NULL)
			unix_error("Calloc error");
		return p;
	}

	void Free(void *ptr)
	{
		free(ptr);
	}

	/****************************
	* Sockets interface wrappers
	****************************/

	int Socket(int domain, int type, int protocol)
	{
		int rc;

		if ((rc = socket(domain, type, protocol)) < 0)
			unix_error("Socket error");
		return rc;
	}

	void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
	{
		int rc;

		if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
			unix_error("Setsockopt error");
	}

	void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
	{
		int rc;

		if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
			unix_error("Bind error");
	}

	void Listen(int s, int backlog)
	{
		int rc;
		if ((rc = listen(s, backlog)) < 0)
			unix_error("Listen error");
	}

	void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
	{
		int rc;

		if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
			unix_error("Connect error");
	}

	/************************
	* DNS interface wrappers
	***********************/

	/* $begin gethostbyname */
	struct hostent *Gethostbyname(const char *name)
	{
		struct hostent *p;

		if ((p = gethostbyname(name)) == NULL)
			dns_error("Gethostbyname error");
		return p;
	}
	/* $end gethostbyname */

	struct hostent *Gethostbyaddr(const char *addr, int len, int type)
	{
		struct hostent *p;

		if ((p = gethostbyaddr(addr, len, type)) == NULL)
			dns_error("Gethostbyaddr error");
		return p;
	}

	/************************************************
	* Wrappers for Pthreads thread control functions
	************************************************/

	void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp,
		void * (*routine)(void *), void *argp)
	{
		int rc;

		if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
			posix_error(rc, "Pthread_create error");
	}

	void Pthread_cancel(pthread_t tid) {
		int rc;

		if ((rc = pthread_cancel(tid)) != 0)
			posix_error(rc, "Pthread_cancel error");
	}

	void Pthread_join(pthread_t tid, void **thread_return) {
		int rc;

		if ((rc = pthread_join(tid, thread_return)) != 0)
			posix_error(rc, "Pthread_join error");
	}

	/* $begin detach */
	void Pthread_detach(pthread_t tid) {
		int rc;

		if ((rc = pthread_detach(tid)) != 0)
			posix_error(rc, "Pthread_detach error");
	}
	/* $end detach */

	void Pthread_exit(void *retval) {
		pthread_exit(retval);
	}

	pthread_t Pthread_self(void) {
		return pthread_self();
	}

	void Pthread_once(pthread_once_t *once_control, void(*init_function)()) {
		pthread_once(once_control, init_function);
	}

	/********************************
	* Client/server helper functions
	********************************/

	/*
	* open_listenfd - open and return a listening socket on port
	*     Returns -1 and sets errno on Unix error.
	*/
	/* $begin open_listenfd */
	int open_listenfd(int port)
	{
		int listenfd, optval = 1;
		struct sockaddr_in serveraddr;

		/* Create a socket descriptor */
		if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) /* 居然没有使用包裹函数 */
			return -1;

		/* Eliminates "Address already in use" error from bind. */
		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
			(const void *)&optval, sizeof(int)) < 0) /* 设置端口复用 */
			return -1;

		/* Listenfd will be an endpoint for all requests to port
		on any IP address for this host */
		bzero((char *)&serveraddr, sizeof(serveraddr));
		serveraddr.sin_family = AF_INET;
		serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serveraddr.sin_port = htons((unsigned short)port);
		if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) /* 绑定 */
			return -1;

		/* Make it a listening socket ready to accept connection requests */
		if (listen(listenfd, LISTENQ) < 0) /* 监听 */
			return -1;
		return listenfd; /* 返回监听套接字 */
	}
	/* $end open_listenfd */


	int Open_listenfd(int port)
	{
		int rc;

		if ((rc = open_listenfd(port)) < 0)
			unix_error("Open_listenfd error");
		return rc;
	}
	/* $end csapp.c */
}




