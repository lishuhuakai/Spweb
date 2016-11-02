#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <list>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <utility>
#include "mutex.h"
#include "condition.h"
#include "csapp.h"
#include "noncopyable.h"
#include "epoll_ulti.h"
using namespace utility;

class ThreadPool : noncopyable
{
public:
	typedef boost::function<void()> Task; /* 需要执行的任务 */
	ThreadPool(int threadNum, int maxTaskNum);
	~ThreadPool();
	bool append(Task&&); /* 往工作队列中添加任务 */
	void run();
	static void* startThread(void* arg); /* 工作者线程 */
private:
	bool isFull();
	Task take();
	size_t queueSize();
	int threadNum_; /* 线程的数目 */
	int maxQueueSize_;
	std::list<Task> queue_; /* 工作队列 */
	MutexLock mutex_;
	Condition notEmpty_;
	Condition notFull_;
};

#endif /* _THREAD_POOL_H_ */
