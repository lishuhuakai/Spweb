#include "http_handle.h"

Cache g_cache;
Cache& HttpHandle::cache_ = g_cache;
int HttpHandle::epollfd_ = -1;
const char HttpHandle::rootDir_[] = "/home/lishuhuakai/WebSiteSrc/html_book_20150808/reference";
const char HttpHandle::homePage_[] = "index.html";

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

void HttpHandle::serveStatic(char *fileName, size_t fileSize)
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
	cache_.getFileAddr(fileName, fileSize, fileInfo_); /* 添加文件 */
	//mylog("fileInfo_ use count: %d", fileInfo_.use_count());
	sendFile_ = true;
}

void HttpHandle::clientError(char *cause, char *errnum, char *shortmsg, char *longmsg)
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
	//mylog("buf: %s", buf);
	//mylog("readBuf: %s", readBuf_);
	//mylog("read:%d", nRead_);
	while (strcmp(buf, "\r\n")) {          //line:netp:readhdrs:checkterm
		getLine(buf, MAXLINE);
		//mylog("buf: %s", buf);
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
		strcpy(filename, rootDir_);                          
		strcat(filename, uri);                          
		if (uri[strlen(uri) - 1] == '/')                  
			strcat(filename, homePage_);               
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
		if (readBuf_[nChecked_++] == '\n')
			break;
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

	if ((false == read()) || (nRead_ == 0)) { /* 对方已经关闭了连接 */
		return STATUS_CLOSE;
	}
	//mylog("processRead() readed");
	//mylog("fileInfo_ use count %d", fileInfo_.use_count());
	/* 接下来开始解析读入的数据 */
	getLine(line, MAXLINE); /* 读取一行数据 */
	/* 使用sscanf函数确实是一个非常棒的办法! */
	sscanf(line, "%s %s %s", method, uri, version);      
	if (strcasecmp(method, "GET")) {               
		clientError(method, "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
	}
	//mylog("processRead() before readRequestHdrs");
	readRequestHdrs();  /* 处理剩余的请求头部 */
	//mylog("processRead() readRequestHdrs");

	is_static = parseUri(uri, filename, cgiargs);
	if (stat(filename, &sbuf) < 0) { 
		clientError(filename, "404", "Not found",
			"Tiny couldn't find this file"); /* 没有找到文件 */
		goto end;
	}                                                   

	if (is_static) { /* Serve static content */
		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clientError(filename, "403", "Forbidden",
				"Tiny couldn't read the file"); /* 权限不够 */
			goto end;
		}
		serveStatic(filename, sbuf.st_size);  
		//mylog("fileInfo_ use count %d", fileInfo_.use_count());
	}
	else { /* Serve dynamic content */
		clientError(method, "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
	}
end:
	setState(kWrite);
	//mylog("fileInfo_ use count %d", fileInfo_.use_count());
	//mylog("begin write!\n");
	return processWrite();
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
	
	setState(kRead); /* 现在需要读数据 */
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
		//mylog("byte_read:%d", byte_read);
		if (byte_read == -1) {  /* 代表出错了 */
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
				break; /* 表示已经读取完毕了! */
			}
			return false; /* 到这里表示出错 */
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
	//mylog("send file! fileInfo_ use count %d", fileInfo_.use_count());
	//size_t bytesToSend = fileInfo_->size_ + strlen(writeBuf_); /* 总共需要发送的字节数目 */
	size_t nRemain = strlen(writeBuf_) - written_; /* writeBuf_中还有多少字节要写 */
	if (nRemain > 0) {
		while (true) {
			nRemain = strlen(writeBuf_) - written_;
			res = write(sockfd_, writeBuf_ + written_, nRemain);
			if (res < 0) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { /* 资源暂时不可用 */
					setState(kWrite); /* 下一步需要写数据 */
					return STATUS_WRITE;
				}
				setState(kError);
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
		assert(fileInfo_);
		//mylog("--> fileInfo_ is not null!");
		//mylog("--> fileInfo_ use count %d", fileInfo_.use_count());
		size_t bytesToSend = fileInfo_->size_ + strlen(writeBuf_); /* 总共需要发送的字节数目 */
		//mylog("bytesToSend! %d", bytesToSend);
		char *fileAddr = (char *)fileInfo_->addr_;
		//mylog("fileAddr %d", fileAddr);
		size_t fileSize = fileInfo_->size_;
		//mylog("fileSize %d", fileSize);
		while (true) {
			size_t offset = written_ - strlen(writeBuf_);
			res = write(sockfd_, fileAddr + offset, fileSize - offset);
			//mylog("file send once!");
			if (res < 0) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { /* 资源暂时不可用 */
					setState(kWrite); /* 下一步需要写数据 */
					return STATUS_WRITE;
				}
				setState(kError); /* 出现了错误 */
				return STATUS_ERROR;
			}
			written_ += res;
			if (written_ == bytesToSend)
				break;
		}
		
	}
	//mylog("send over!");
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

void HttpHandle::process()
{
	int res;
	switch (state_)
	{
	case kRead: {
		res = processRead(); 
		//mylog("processRead res is %d", res);
		if (res == STATUS_WRITE)
			modfd(epollfd_, sockfd_, EPOLLOUT);
		else
			removefd(epollfd_, sockfd_);
		break;
	}
	case kWrite: {
		res = processWrite(); 
		//mylog("processWrite res is %d", res);
		if (res == STATUS_READ)
			modfd(epollfd_, sockfd_, EPOLLIN);
		else
			removefd(epollfd_, sockfd_);
		break;
	}
	default:
		removefd(epollfd_, sockfd_);
		break;
	}
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