#include "http_handle.h"

Cache g_cache;
extern char* root_dir;
extern char* home_page;
Cache& HttpHandle::cache_ = g_cache;

void HttpHandle::getFileType(char *filename, char *filetype)
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

void HttpHandle::serveStatic(char *fileName, int fileSize)
{ /* 用于处理静态的网页 */
	int srcfd;
	char fileType[MAXLINE], buf[MAXBUF];

	/* Send response headers to client */
	getFileType(fileName, fileType);   
	sprintf(buf, "HTTP/1.0 200 OK\r\n");
	sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
	sprintf(buf, "%sContent-length: %d\r\n", buf, fileSize);
	sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, fileType);
	addResponse(buf);
	fileInfo_ = cache_.getFileAddr(fileName, fileSize); /* 添加文件 */
	sendFile_ = true;
}

void HttpHandle::clienterror(char *cause, char *errnum, char *shortmsg, char *longmsg)
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

void HttpHandle::readRequestHdrs()
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

int HttpHandle::parseUri(char *uri, char *filename, char *cgiargs)
{
	char *ptr;

	if (!strstr(uri, "mwiki")) {  
		strcpy(cgiargs, "");                            
		strcpy(filename, root_dir);                          
		strcat(filename, uri);                          
		if (uri[strlen(uri) - 1] == '/')                  
			strcat(filename, home_page);               
		//printf("fileName: %s\n", filename);
		return 1;
	}
	else {  /* Dynamic content */ 
		ptr = index(uri, '?');
		if (ptr) {
			strcpy(cgiargs, ptr + 1);
			*ptr = '\0';
		}
		else
			strcpy(cgiargs, ""); 
		strcpy(filename, ".");   
		strcat(filename, uri);
		return 0;
	}
}

int HttpHandle::getLine(char *buf, int maxsize) 
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

int HttpHandle::processRead()
{
	int is_static;
	struct stat sbuf;
	char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
	char filename[MAXLINE], cgiargs[MAXLINE];
	char line[MAXLINE];

	if (false == read()) { /* 对方已经关闭了连接 */
		return STATUS_CLOSE;
	}

	/* 接下来开始解析读入的数据 */
	getLine(line, MAXLINE); /* 读取一行数据 */
	/* 使用sscanf函数确实是一个非常棒的办法! */
	sscanf(line, "%s %s %s", method, uri, version);      
	if (strcasecmp(method, "GET")) {               
		clienterror(method, "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
	}
	readRequestHdrs();  /* 处理剩余的请求头部 */
													 /* Parse URI from GET request */
	is_static = parseUri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) { 
		clienterror(filename, "404", "Not found",
			"Tiny couldn't find this file"); /* 没有找到文件 */
		goto end;
	}                                                   

	if (is_static) { /* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clienterror(filename, "403", "Forbidden",
				"Tiny couldn't read the file"); /* 权限不够 */
			goto end;
		}
		serveStatic(filename, sbuf.st_size);       
	}
	else { /* Serve dynamic content */
		clienterror(method, "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
		
		// not use!
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
			clienterror(filename, "403", "Forbidden",
				"Tiny couldn't run the CGI program"); /* 访问某个程序 */
			goto end;
		}
		serveDynamic(filename, cgiargs);
	}
	end:
	return processWrite();
}


void HttpHandle::serveDynamic(char *filename, char *cgiargs) /* 要获取动态的内容 */
{
	char buf[MAXLINE], *emptylist[] = { NULL };

	/* Return first part of HTTP response */
	addResponse("HTTP/1.0 200 OK\r\n");
	addResponse("Server: Tiny Web Server\r\n");
	addResponse("\r\n");
	return;

	if (Fork() == 0) { /* child */ 
		/* Real server would set all CGI vars here */
		setenv("QUERY_STRING", cgiargs, 1); 
		Dup2(sockfd_, STDOUT_FILENO);         /* Redirect stdout to client */ 
		Execve(filename, emptylist, environ); /* Run CGI program */
	}
	Wait(NULL); /* Parent waits for and reaps child */
}

void HttpHandle::init(int sockfd)
{
	sockfd_ = sockfd; /* 记录下连接的socket */
	
	/* 如下两行是为了避免TIME_WAIT状态,仅仅用于调试,实际使用时应该去掉 */
	int reuse = 1;
	Setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)); /* 设置端口重用? */
	reset();
}

void HttpHandle::reset()
{
	nRead_ = 0;
	nChecked_ = 0;
	nStored_ = 0;
	keepAlive_ = false;
	sendFile_ = false;
	written_ = 0;
	fileInfo_.reset();
	memset(readBuf_, 0, READ_BUFFER_SIZE);
	memset(writeBuf_, 0, WRITE_BUFFER_SIZE);
}

bool HttpHandle::read()
{ /* 由于epoll设置成了是边缘触发,所以要一次性将数据全部读尽 */
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

int HttpHandle::processWrite()
{
	int res;
	/*- 
	* 数据要作为两部分发送,第1步,要发送writeBuf_里面的数据.
	*/
	int nRemain = strlen(writeBuf_) - written_; /* writeBuf_中还有多少字节要写 */
	if (nRemain > 0) {
		while (true) {
			nRemain = strlen(writeBuf_) - written_;
			res = write(sockfd_, writeBuf_ + written_, nRemain);
			if (res < 0) {
				if (errno == EAGAIN) { /* 资源暂时不可用 */
					return STATUS_WRITE;
				}
				return STATUS_ERROR;
			}
			written_ += res;
			if (written_ == strlen(writeBuf_))
				break;
		}
	}
		
	/*-
	* 第2步,要发送html网页数据.
	*/
	if (sendFile_) {
		int bytesToSend = fileInfo_->size_ + strlen(writeBuf_); /* 总共需要发送的字节数目 */
		while (true) {
			int offset = written_ - strlen(writeBuf_);
			res = write(sockfd_, (char *)fileInfo_->addr_ + offset, fileInfo_->size_ - offset);
			if (res < 0) {
				if (errno == EAGAIN) { /* 资源暂时不可用 */
					return STATUS_WRITE;
				}
				return STATUS_ERROR;
			}
			written_ += res;
			if (written_ == bytesToSend)
				break;
		}
		
	}

	/* 数据发送完毕 */
	if (keepAlive_) { /* 如果需要保持连接的话 */
		reset();
		return STATUS_READ;
	}
	else {
		reset();
		return STATUS_SUCCESS;
	}	
}

bool HttpHandle::addResponse(const char* format, ...)
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

bool HttpHandle::addResponse(char* const str)
{
	int len = strlen(str);
	if ((nStored_ >= WRITE_BUFFER_SIZE) || (nStored_ + len >= WRITE_BUFFER_SIZE)) {
		return false;
	}
	strncpy(writeBuf_ + nStored_, str, len); /* 拷贝len个字符 */
	nStored_ += len; /* nStored_是写缓冲区的末尾 */
	return true;
}