#include "http_handle.h"

extern char* root_dir;
extern char* home_page;

void http_handle::getFileType(char *filename, char *filetype)
{ /* 获得文件的类型 */
	if (strstr(filename, ".html"))
		strcpy(filetype, "text/html");
	else if (strstr(filename, ".gif"))
		strcpy(filetype, "image/gif");
	else if (strstr(filename, ".jpg"))
		strcpy(filetype, "image/jpeg");
	else if (strstr(filename, ".png"))
		strcpy(filetype, "image/png");
	else if (strstr(filename, ".css"))
		strcpy(filetype, "text/css");
	else if (strstr(filename, ".ttf") || strstr(filename, ".otf"))
		strcpy(filetype, "application/octet-stream");
	else
		strcpy(filetype, "text/plain");
}

void http_handle::serveStatic(char *filename)
{ /* 用于处理静态的网页 */
	int srcfd;
	char filetype[MAXLINE], buf[MAXBUF];
	//printf("---------\n");
	//printf("%s\n", filename);
	//printf("---------\n");
	/* Send response headers to client */
	getFileType(filename, filetype);   
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, fileSize_);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
	addResponse(buf);   

	srcfd = Open(filename, O_RDONLY, 0); /* 打开文件 */
	char* srcp = (char *)Mmap(0, fileSize_, PROT_READ, MAP_PRIVATE, srcfd, 0); /* 实现映射 */
	Close(srcfd); /* 关闭文件 */
	copyData(srcp, fileSize_);
	Munmap(srcp, fileSize_);
}

void http_handle::clienterror(char *cause, char *errnum, char *shortmsg, char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];
	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	addResponse("HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	addResponse("Content-type: text/html\r\n");
	addResponse("Content-length: %d\r\n\r\n", (int)strlen(body));
	addResponse("%s", body);
}

void http_handle::readRequestHdrs()
{
	char buf[MAXLINE];
	getLine(buf, MAXLINE);
	while (strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
		getLine(buf, MAXLINE);
		if (strstr(buf, "keep-alive")) {
			keepAlive_ = true; /* 保持连接 */
		}
	}
	return;
}

int http_handle::parseUri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;

	if (!strstr(uri, "mwiki")) {  /* Static content */ //line:netp:parseuri:isstatic
		strcpy(cgiargs, "");                             //line:netp:parseuri:clearcgi
		strcpy(filename, root_dir);                           //line:netp:parseuri:beginconvert1
		strcat(filename, uri);                           //line:netp:parseuri:endconvert1
		if (uri[strlen(uri) - 1] == '/')                   //line:netp:parseuri:slashcheck
			strcat(filename, home_page);               //line:netp:parseuri:appenddefault
		//printf("filename: %s\n", filename);
		return 1;
	}
	else {  /* Dynamic content */                        //line:netp:parseuri:isdynamic
		ptr = index(uri, '?');                           //line:netp:parseuri:beginextract
		if (ptr) {
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		}
		else
			strcpy(cgiargs, "");                         //line:netp:parseuri:endextract
		strcpy(filename, ".");                           //line:netp:parseuri:beginconvert2
		strcat(filename, uri);                           //line:netp:parseuri:endconvert2
		return 0;
	}
}

int http_handle::getLine(char *buf, int maxsize) 
{ /* 用于读取一行数据 */
	int n; /* 用于记录读取的字节的数目 */
	for (n = 0; nChecked_ < nRead_;) {
		*buf++ = readBuf_[nChecked_];
		n++;
		if (readBuf_[nChecked_++] == '\n') {
			break;
		}
	}
	*buf = 0;
	return n;
}


ResState http_handle::processRead()
{
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	char line[MAXLINE];

	if (false == read()) { /* 对方已经关闭了连接 */
		return NEED_CLOSE;
	}

	/* 接下来开始解析读入的数据 */
	getLine(line, MAXLINE); /* 读取一行数据 */
	/* 使用sscanf函数确实是一个非常棒的办法! */
	sscanf(line, "%s %s %s", method, uri, version);       //line:netp:doit:parserequest
	if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
		clienterror(method, "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
	}                                                    //line:netp:doit:endrequesterr
	readRequestHdrs();  /* 处理剩余的请求头部 */
													 /* Parse URI from GET request */
	is_static = parseUri(uri, filename, cgiargs);       //line:netp:doit:staticcheck
	if (stat(filename, &sbuf) < 0) {                     //line:netp:doit:beginnotfound
		clienterror(filename, "404", "Not found",
			"Tiny couldn't find this file"); /* 没有找到文件 */
		goto end;
	}                                                    //line:netp:doit:endnotfound

	fileSize_ = sbuf.st_size; /* 记录下文件的大小 */

	if (is_static) { /* Serve static content */
		

		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) { //line:netp:doit:readable
			clienterror(filename, "403", "Forbidden",
				"Tiny couldn't read the file"); /* 权限不够 */
			goto end;
		}
		serveStatic(filename);        // line:netp:doit:servestatic
	}
	else { /* Serve dynamic content */

		clienterror(method, "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;

		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { // line:netp:doit:executable
			clienterror(filename, "403", "Forbidden",
				"Tiny couldn't run the CGI program"); /* 访问某个程序 */
			goto end;
		}
		serveDynamic(filename, cgiargs); // line:netp:doit:servedynamic
	}
	end:
	return processWrite();
}


/* $begin serve_dynamic */
void http_handle::serveDynamic(char *filename, char *cgiargs) /* 要获取动态的内容 */
{
	//printf("==> I am in serve_dynamic!\n");
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	addResponse("HTTP/1.0 200 OK\r\n");
	addResponse("Server: Tiny Web Server\r\n");
	addResponse("\r\n");
	return;

	if (Fork() == 0) { /* child */ 
		//line:netp:servedynamic:fork
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1); // line:netp:servedynamic:setenv
		Dup2(sockfd_, STDOUT_FILENO);         /* Redirect stdout to client */ //line:netp:servedynamic:dup2
		Execve(filename, emptylist, environ); /* Run CGI program */ //line:netp:servedynamic:execve
	}
	Wait(NULL); /* Parent waits for and reaps child */ //line:netp:servedynamic:wait
}

void http_handle::init(int sockfd)
{
	sockfd_ = sockfd; /* 记录下连接的socket */
					  /* 如下两行是为了避免TIME_WAIT状态,仅仅用于调试,实际使用时应该去掉 */
	int reuse = 1;
	Setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); /* 设置端口重用? */
	resetData();
}

void http_handle::resetData()
{
	nRead_ = 0;
	nChecked_ = 0;
	nStored_ = 0;
	fileAddress_ = 0;
	fileSize_ = 0;
	keepAlive_ = false;
	written_ = 0;
	memset(readBuf_, 0, READ_BUFFER_SIZE);
	memset(writeBuf_, 0, WRITE_BUFFER_SIZE);
}


bool http_handle::read()
{
	/* 由于epoll设置成了是边缘触发,所以要一次性将数据全部读尽 */
	nRead_ = 0; /* 首先要清零 */
	nChecked_ = 0;
	if (nRead_ >= READ_BUFFER_SIZE) {
		return false;
	}
	int byte_read = 0;
	while (true) {
		byte_read = recv(sockfd_, readBuf_ + nRead_, READ_BUFFER_SIZE - nRead_, 0);
		if (byte_read == -1) {  /* 代表出错了 */
			break;
		}
		else if (byte_read == 0) { /* 对方已经关闭了连接 */
			return false;
		}
		nRead_ += byte_read;
	}
	return true;
}

ResState http_handle::processWrite()
{
	int temp = 0;
	int bytes_hava_send = 0;
	
	int bytes_to_send = nStored_ - written_; /* 需要发送的字节数目 */
	//printf("processWrite() --> bytes_to_send : %d\n", bytes_to_send);
	if (bytes_to_send == 0) { /* 已经发送完毕了 */
		//printf("processWrite() --> sending over! keep reading!\n");
		resetData();
		return SUCCESS; 
	}
	assert(written_ == 0);
	while (true) {
		int temp = write(sockfd_, writeBuf_ + written_, bytes_to_send);
		if (temp <= -1) {
			/* 如果TCP写缓冲没有空间,那么需要等待 */
			if (errno == EAGAIN) { /* 需要注意的是,这个m_sockfd可是非阻塞版本的 */
				//printf("processWrite() --> Keep writing!\n");
				return NEED_WRITE; 
			}
			return ERROR; /* 发送失败,我们要关闭连接 */
		}

		bytes_to_send -= temp;
		written_ += temp; /* 已经发送了的比特数 */
		if (bytes_to_send == 0) { /* 已经发送完毕了 */
			/* 发送HTTP响应成功,根据HTTP请求中的Connection字段决定是否立即关闭连接 */
			resetData();
			//printf("processWrite() --> Send over!\n");
			if (!keepAlive_) {
				//printf("processWrite() --> Close connection!\n");
				return SUCCESS;
			}
			else {
				//printf("Keep alive!\n");
				return NEED_READ; /* 返回一个false是要我们关闭连接吗? */
			}	
		}
	}
}

bool http_handle::addResponse(const char* format, ...)
{
	if (nStored_ >= WRITE_BUFFER_SIZE) {
		return false;
	}
	va_list arg_list;
	va_start(arg_list, format);
	int len = vsnprintf(writeBuf_ + nStored_, WRITE_BUFFER_SIZE - 1 - nStored_, format, arg_list);
	if (len >= (WRITE_BUFFER_SIZE - 1 - nStored_)) {
		return false;
	}
	nStored_ += len;
	va_end(arg_list);
	return true;
}

bool http_handle::addResponse(char* const str)
{
	int len = strlen(str);
	if ((nStored_ >= WRITE_BUFFER_SIZE) || (nStored_ + len >= WRITE_BUFFER_SIZE)) {
		return false;
	}
	strncpy(writeBuf_ + nStored_, str, len); /* 拷贝len个字符 */
	nStored_ += len; /* nStored_是写缓冲区的末尾 */
	return true;
}

bool http_handle::copyData(void* src, int len)
{
	if ((nStored_ >= WRITE_BUFFER_SIZE) || (nStored_ + len >= WRITE_BUFFER_SIZE)) {
		return false;
	}

	strncpy(writeBuf_ + nStored_, (char *)src, len); /* 拷贝len个字符 */
	nStored_ += len; /* nStored_是写缓冲区的末尾 */
	return true;
}