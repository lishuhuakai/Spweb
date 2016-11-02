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
	FileInfo(std::string& fileName, int fileSize) {
		int srcfd = Open(fileName.c_str(), O_RDONLY, 0); /* 打开文件 */
		size_ = fileSize;
		addr_ = Mmap(0, fileSize, PROT_READ, MAP_PRIVATE, srcfd, 0);
		Close(srcfd); /* 关闭文件 */
	}
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
	boost::shared_ptr<FileInfo> getFileAddr(std::string fileName, int fileSize) {
		if (cache_.end() != cache_.find(fileName)) { /* 如果在cache中找到了 */
			return cache_[fileName];
		}
		if (cache_.size() >= MAX_CACHE_SIZE) { /* 文件数目过多,需要删除一个元素 */
			cache_.erase(cache_.begin()); /* 直接移除掉最前一个元素 */
		}
		/* 没有找到的话,我们需要加载文件 */
		boost::shared_ptr<FileInfo> fileInfo(new FileInfo(fileName, fileSize));
		cache_[fileName] = fileInfo;
		return fileInfo;
	}
	/* 线程安全版本的getFileAddr */
	void getFileAddr(std::string fileName, int fileSize, boost::shared_ptr<FileInfo>& ptr) {
		/*-
		* shared_ptr并不是线程安全的,对其的读写都需要加锁.
		*/
		MutexLockGuard lock(mutex_);
		if (cache_.end() != cache_.find(fileName)) { /* 如果在cache中找到了 */
			ptr = cache_[fileName];
			return;
		}
		if (cache_.size() >= MAX_CACHE_SIZE) { /* 文件数目过多,需要删除一个元素 */
			cache_.erase(cache_.begin()); /* 直接移除掉最前一个元素 */
		}
		boost::shared_ptr<FileInfo> fileInfo(new FileInfo(fileName, fileSize));
		cache_[fileName] = fileInfo;
		ptr = std::move(fileInfo); /* 直接使用move语义 */
	}

};

#endif /* _CACHE_H_ */
