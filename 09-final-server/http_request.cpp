#include "http_request.h"
#include "buff.h"

const char HttpRequest::rootDir_[] = "/home/lishuhuakai/WebSiteSrc/html_book_20150808/reference";
const char HttpRequest::homePage_[] = "index.html";

void HttpRequest::reset() /* 清零 */
{
	state_ = kExpectRequestLine;
	path_.clear();
}

HttpRequest::HttpRequestParseState HttpRequest::parseRequest(Buffer& buf)
{
	bool ok = true;
	bool hasMore = true;
	while (hasMore) {
		if (state_ == kExpectRequestLine) {
			const char* crlf = buf.findCRLF(); /* 找到回车换行符 */
			if (crlf) { /* 如果找到了! */
				ok = processRequestLine(buf);
			}
			else {
				hasMore = false; /* 没有找到,可能表示对方还没有发送完整的一行数据,要继续去监听客户的写事件 */
			}

			if (ok) 
				state_ = kExpectHeaders;
			else {
				state_ = kError; /* 出现错误 */
				hasMore = false;
			}
		}
		else if (state_ == kExpectHeaders) { /* 处理头部的信息 */
			if (true == (ok = processHeaders(buf))) {
				state_ = kGotAll;
				hasMore = false;
			}
			else {
				hasMore = false;
			}	
		}
		else if (state_ == kExperctBody) { /* 暂时还未实现 */
			
		}
	}
	return state_;
}

const char* HttpRequest::getFileType()
{ /* 获得文件的类型 */
	if (strstr(path_.c_str(), ".html"))
		fileType_ = "text/html";
	else if (strstr(path_.c_str(), ".gif"))
		fileType_ = "image/gif";
	else if (strstr(path_.c_str(), ".jpg"))
		fileType_ = "image/jpeg";
	else if (strstr(path_.c_str(), ".png"))
		fileType_ = "image/png";
	else if (strstr(path_.c_str(), ".css"))
		fileType_ = "text/css";
	else if (strstr(path_.c_str(), ".ttf") || strstr(path_.c_str(), ".otf"))
		fileType_ = "application/octet-stream";
	else
		fileType_ =  "text/plain";
	return fileType_.c_str();
}

bool HttpRequest::processHeaders(Buffer& buf) /* 处理其余的头部信息 */
{ /* 其余的玩意,我就不处理啦! */
	char line[1024];
	char key[256], value[256];
	while (buf.getLine(line, sizeof line)) {
		if (strlen(line) == 0) { /* 只有取到了最后一个才能返回true */
			return true;
		}
		if (strstr(line, "keep-alive")) {
			keepAlive_ = true; /* 保持连接 */
		}

	}
	return false;
}

void HttpRequest::setMethod(const char* start, size_t len)
{
	method_.assign(start, start + len);
}

void HttpRequest::setPath(const char* uri, size_t len)
{
	char *ptr;
	if (!strstr(uri, "mwiki")) {
		path_ += rootDir_;
		path_ += uri;
		if (uri[strlen(uri) - 1] == '/')
			path_ += homePage_;
		static_ = true; /* 请求的是静态文件 */
		return;
	}
	else {  /* Dynamic content */
		/* 暂时不支持动态文件 */
		static_ = false;
		return;
	}
}

bool HttpRequest::processRequestLine(Buffer& buf)
{
	bool succeed = false;
	char line[256];
	char method[64], path[256], version[64];
	buf.getLine(line, sizeof line); 
	sscanf(line, "%s %s %s", method, path, version);
	setMethod(method, strlen(method));
	setPath(path, strlen(path));
	/* version就不处理了 */
	return true;
}