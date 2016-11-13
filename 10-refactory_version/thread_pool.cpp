#include "thread_pool.h"
#include "mutex.h"

ThreadPool::ThreadPool(int threadNum, int maxQueueSize)
	: threadNum_(threadNum)
	, maxQueueSize_(maxQueueSize)
	, mutex_()
	, notEmpty_(mutex_)
	, notFull_(mutex_)

{
	assert(threadNum >= 1 && maxQueueSize >= 1);
	/* 接下来要构建threadNum个线程 */
	pthread_t tid_t;
	for (int i = 0; i < threadNum; i++) {
		Pthread_create(&tid_t, NULL, startThread, this);
	}
}

size_t ThreadPool::queueSize()
{
	MutexLockGuard lock(mutex_);
	return queue_.size();
}

ThreadPool::~ThreadPool() {

}

void* ThreadPool::startThread(void* obj)
{ /* 工作者线程 */
	Pthread_detach(Pthread_self());
	ThreadPool* pool = static_cast<ThreadPool*>(obj);
	pool->run();
	return pool;
}

ThreadPool::Task ThreadPool::take()
{
	MutexLockGuard lock(mutex_);
	while (queue_.empty()) {
		notEmpty_.wait();
	}
	Task task;
	if (!queue_.empty()) {
		task = queue_.front();
		queue_.pop_front();
		if (maxQueueSize_ > 0) {
			notFull_.notify();
		}
	}
	//mylog("threadpool take 1 task!");
	return task;
}
void ThreadPool::run()
{
	for ( ; ; ) { /* 一直运行下去 */
		Task task(take());
		if (task) {
			//mylog("task run!");
			task();
		}
		//mylog("task over!");
	}
}

bool ThreadPool::isFull()
{
	mutex_.assertLocked();
	return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

bool ThreadPool::append(Task&& task)
{ /* 使用了右值引用 */
	{
		MutexLockGuard lock(mutex_); /* 首先加锁 */
		while (isFull()) {
			notFull_.wait(); /* 等待queue有空闲位置 */
		}
		assert(!isFull());
		queue_.push_back(std::move(task)); /* 直接用move语义,提高了效率 */
		//mylog("put task onto queue!");
	}
	notEmpty_.notify(); /* 通知任务队列中有任务可做了 */
}
