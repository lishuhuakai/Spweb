#include "http_handle.h"
#include "cache.h"

Cache g_cache; /* 全局变量最好拿出一个单独的文件存放,当然,我们这个项目太小,这么干也没啥. */
Cache& HttpHandle::cache_ = g_cache;
int HttpHandle::epollfd_ = -1;

void HttpHandle::serveStatic(const char *fileName, size_t fileSize)
{ /* 用于处理静态的网页 */
	/* Send response headers to client */
	writeBuffer_.appendStr("HTTP/1.0 200 OK\r\n");
	writeBuffer_.appendStr("Server: Tiny Web Server\r\n");
	writeBuffer_.appendStr("Content-length: %d\r\n", fileSize);
	writeBuffer_.appendStr("Content-type: %s\r\n\r\n", request_.getFileType());
	cache_.getFileAddr(fileName, fileSize, fileInfo_); /* 添加文件 */
	sendFile_ = true;
}

void HttpHandle::clientError(const char *cause, const char *errnum, const char *shortmsg, const char *longmsg)
{
	char buf[MAXLINE], body[MAXBUF];

	/* Build the HTTP response body */
	sprintf(body, "<html><title>Tiny Error</title>");
	sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
	sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
	sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
	sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

	/* Print the HTTP response */
	writeBuffer_.appendStr("HTTP/1.0 %s %s\r\n", errnum, shortmsg);
	writeBuffer_.appendStr("Content-type: text/html\r\n");
	writeBuffer_.appendStr("Content-length: %d\r\n\r\n", (int)strlen(body));
	writeBuffer_.appendStr("%s", body);
}


void HttpHandle::processRead()
{
	struct stat sbuf;

	if (!read()) { /* 读取失败,代表对方关闭了连接 */
		state_ = kClosed;
		return;
	}
	/*-
	* 试想这样一种情况,对方并没有一次性将request发送完毕,而是只发送了一部分,你应该如何来处理?正确的方式是继续去读,直到读到结尾符为止.
	* 当然,我这里并没有处理request是错误的情况,这里假设request都是正确的,否则的话,就要关闭连接了.
	*/
	RequestParseState state = request_.parseRequest(readBuffer_);
	if (state == RequestParseState::kError)  { /* 如果处理不成功,就要返回 */
		state_ = kError; /* 解析出错 */
		return;
	}
	else if (state != RequestParseState::kGotAll){ /* 没出错的话,表明对方只发送了request的一部分,我们需要继续读 */
		return;
	}
	
	if (strcasecmp(request_.method_.c_str(), "GET")) {  /* 只支持Get方法 */         
		clientError(request_.method_.c_str(), "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
	}
	                                     
	if (request_.static_) { /* 只支持静态网页 */
		if (stat(request_.path_.c_str(), &sbuf) < 0) {
			clientError(request_.path_.c_str(), "404", "Not found",
				"Tiny couldn't find this file"); /* 没有找到文件 */
			goto end;
		}

		if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
			clientError(request_.path_.c_str(), "403", "Forbidden",
				"Tiny couldn't read the file"); /* 权限不够 */
			goto end;
		}
		serveStatic(request_.path_.c_str(), sbuf.st_size);
		
	}
	else { /* Serve dynamic content */
		clientError(request_.method_.c_str(), "501", "Not Implemented",
			"Tiny does not implement this method");
		goto end;
	}
end:
	state_ = kExpectWrite;
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
	keepAlive_ = false;
	sendFile_ = false;
	fileWritten_ = 0;
	readBuffer_.retrieveAll();
	writeBuffer_.retrieveAll();
	request_.reset();
	state_ = kExpectRead;
}

bool HttpHandle::read()
{ /* 由于epoll设置成了是边缘触发,所以要一次性将数据全部读尽 */
	int byteRead = 0;
	while (true) {
		int savedErrno;
		byteRead = readBuffer_.readFd(sockfd_, &savedErrno);
		if (byteRead == -1) {  /* 代表出错了 */
			if ((savedErrno == EAGAIN) || (savedErrno == EWOULDBLOCK)) {
				break; /* 表示已经读取完毕了! */
			}
			state_ = kError; /* 到这里表示出错 */
			return false; 
		}
		else if (byteRead == 0) { /* 对方已经关闭了连接 */
			state_ = kClosed;
			return false;
		}
	}
	return true;
}

void HttpHandle::processWrite()
{
	int res;
	/*- 
	* 数据要作为两部分发送,第1步,要发送writeBuf_里面的数据.
	*/
	size_t nRemain = writeBuffer_.readableBytes(); /* writeBuf_中还有多少字节要写 */
	if (nRemain > 0) {
		while (true) {
			size_t len = writeBuffer_.readableBytes();
			//mylog("1. len = %ld", len);
			res = write(sockfd_, writeBuffer_.peek(), len);
			if (res < 0) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { /* 资源暂时不可用 */
					return;
				}
				state_ = kError;
				return;
			}
			//mylog("2. retrive begin");
			//mylog("3. res = %d, readableBytes() = %ld", res, writeBuffer_.readableBytes());
			writeBuffer_.retrieve(res);
			//mylog("4. retrive ok, readableBytes() = %ld", writeBuffer_.readableBytes());
			if (writeBuffer_.readableBytes() == 0) break;
		}
	}
		
	/*-
	* 第2步,要发送html网页数据.
	*/
	if (sendFile_) {
		char *fileAddr = (char *)fileInfo_->addr_;
		size_t fileSize = fileInfo_->size_;
		while (true) {
			res = write(sockfd_, fileAddr + fileWritten_, fileSize - fileWritten_);
			if (res < 0) {
				if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) { /* 资源暂时不可用 */
					return;
				}
				state_ = kError; /* 出现了错误 */
				return;
			}
			fileWritten_ += res;
			if (fileWritten_ == fileInfo_->size_)
				break;	
		}
		
	}

	/* 数据发送完毕 */
	reset();
	if (keepAlive_) /* 如果需要保持连接的话 */
		state_ = kExpectRead;
	else
		state_ = kSuccess;
}


void HttpHandle::process()
{
	/*-
	* 在process之前,只有这么一些状态kExpectRead, kExpectWrite
	*/
	switch (state_)
	{
	case kExpectRead: {
		processRead(); 
		break;
	}
	case kExpectWrite: {
		processWrite(); 
		break;
	}
	default: /* 成功,失败,这些都需要关闭连接 */
		removefd(epollfd_, sockfd_);
		break;
	}
	/*- 
	* 程序执行完成后,有这么一些状态kExpectRead, kExpectWrite, kError, kSuccess
	*/
	switch (state_)
	{
	case kExpectRead: {
		modfd(epollfd_, sockfd_, EPOLLIN, true);
		break;
	}
	case kExpectWrite: {
		modfd(epollfd_, sockfd_, EPOLLOUT, true);
		break;
	}
	default: {
		removefd(epollfd_, sockfd_);
		break;
	}
	}
}
