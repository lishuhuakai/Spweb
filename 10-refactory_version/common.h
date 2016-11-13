#ifndef _COMMOM_H_
#define _COMMOM_H_
#include <memory>
/*-
* 我想做的一件事情就是将全局使用的一些别名都放在这个文件里面.
*/
class FileInfo;
using pInfo = std::shared_ptr<FileInfo>;

enum class RequestParseState {
	kExpectRequestLine,
	kExpectHeaders,
	kExperctBody,
	kGotAll,
	kError
};


#endif
