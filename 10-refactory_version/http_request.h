#ifndef _HTTP_REQUEST_H_
#define _HTTP_REQUEST_H_
#include <string>
#include "noncopyable.h"
#include "common.h"

class Buffer;

class HttpRequest : noncopyable
{
public:
	
	
	HttpRequest()
		: state_(RequestParseState::kExpectRequestLine)
	{}

	RequestParseState parseRequest(Buffer& buf);
	bool processRequestLine(Buffer& buf);
	bool processHeaders(Buffer& buf);
private:
	RequestParseState state_;
public:
	bool keepAlive_;
	bool sendFile_;
	bool static_; /* 是否为静态页面 */
	std::string method_;
	std::string path_;
	std::string fileType_;
	const char* getFileType();
	void reset();
private:
	static const char rootDir_[]; /* 网页的根目录 */
	static const char homePage_[]; /* 所指代的网页 */
	void setMethod(const char* start, size_t len);
	void setPath(const char* start, size_t len);
};

#endif
