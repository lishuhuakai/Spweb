#ifndef _CACHE_H_
#define _CACHE_H_

#include <map>
#include <string>
#include <boost/shared_ptr.hpp>
#include "csapp.h"
#include "noncopyable.h"
#include "mutex.h"
using namespace utility;


class FileInfo : noncopyable
{
public:
	FileInfo(std::string& fileName, int fileSize);
	~FileInfo() {
		Munmap(addr_, size_); /* 解除映射 */
	}
public:
	void *addr_; /* 地址信息 */
	int size_; /* 文件大小 */

};

class Cache : noncopyable
{
	typedef std::map<std::string, boost::shared_ptr<FileInfo>>::iterator it;
private:
	std::map<std::string, boost::shared_ptr<FileInfo>> cache_; /* 实现文件名称到地址的一个映射 */
	static const size_t MAX_CACHE_SIZE = 100; /* 最多缓存100个文件 */
	MutexLock mutex_;
	
public:
	Cache()
		: mutex_()
	{}
	/* 下面的版本线程不安全 */
	boost::shared_ptr<FileInfo> getFileAddr(std::string fileName, int fileSize);
	/* 线程安全版本的getFileAddr */
	void getFileAddr(std::string fileName, int fileSize, boost::shared_ptr<FileInfo>& ptr);

};

#endif /* _CACHE_H_ */
