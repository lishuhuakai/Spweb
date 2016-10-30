#ifndef _MUTEX_H_
#define _MUTEX_H_
/*-
 * 参照muduo库的mutex写出的一个简易的Mutex类.
 */

#include <assert.h>
#include <pthread.h>
#include "noncopyable.h"


 // 让我来解析一下这个宏究竟干了一些什么东西。
 // 首先定义了一个ret类型的变量errnum,然后值为ret
 // __builtin_expect宏，就是预测errnum!=0的概率是比较小的，所以gcc会帮你做一些优化
 // 抛弃这个宏的话，其实就是想表达if(errnum != 0) ..一般来说表示发生了错误
 // 然后就是要做一些处理吧！
#define MCHECK(ret) ({ __typeof__ (ret) errnum = (ret);         \
                       if (__builtin_expect(errnum != 0, 0))    \
                         __assert_perror_fail (errnum, __FILE__, __LINE__, __func__);})

class MutexLock : noncopyable
{
public:
	MutexLock()
		: holder_(0)
	{
		MCHECK(pthread_mutex_init(&mutex_, NULL)); /* 初始化 */
	}
	~MutexLock()
	{
		assert(holder_ == 0);
		MCHECK(pthread_mutex_destroy(&mutex_)); /* 销毁锁 */
	}

	bool isLockedByThisThread() const /* 测试锁是否被本线程持有 */
	{
		return holder_ == pthread_self();
	}

	void assertLocked() const
	{
		assert(isLockedByThisThread());
	}

	void lock()
	{
		MCHECK(pthread_mutex_lock(&mutex_));
		assignHolder(); /* 指定拥有者 */
	}

	void unlock()
	{
		unassignHolder(); /* 丢弃拥有者 */
		MCHECK(pthread_mutex_unlock(&mutex_));
	}

	pthread_mutex_t* getPthreadMutex()
	{
		return &mutex_;
	}
private:
	friend class Condition;
	class UnassignGuard : noncopyable /* 类中的一个类 */
	{
	public:
		UnassignGuard(MutexLock& owner) /* 这个类貌似没有什么用处啊! */
			: owner_(owner)
		{
			owner_.unassignHolder();
		}
		~UnassignGuard()
		{
			owner_.assignHolder();
		}
	private:
		MutexLock& owner_;
	};

	void unassignHolder()
	{
		holder_ = 0;
	}
	void assignHolder()
	{
		holder_ = pthread_self();
	}
	pthread_mutex_t mutex_;
	pid_t holder_;
};

class MutexLockGuard : noncopyable
{
public:
	explicit MutexLockGuard(MutexLock& mutex)
		: mutex_(mutex)
	{
		mutex_.lock();
	}
	~MutexLockGuard()
	{
		mutex_.unlock();
	}
private:
	MutexLock& mutex_;
};


#endif /* _MUTEX_H_ */
