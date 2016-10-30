#ifndef _CONDITION_H_
#define _CONDITION_H_

#include <pthread.h>
#include "mutex.h"
#include "noncopyable.h"

/*-
* 参照muduo库的condition写的一个简易的condition类.
*/

class Condition : noncopyable
{
public:
	explicit Condition(MutexLock& mutex)
		: mutex_(mutex)
	{
		MCHECK(pthread_cond_init(&pcond_, NULL));
	}
	~Condition()
	{
		MCHECK(pthread_cond_destroy(&pcond_)); /* 销毁条件变量 */
	}
	void wait()
	{
		MutexLock::UnassignGuard ug(mutex_);
		MCHECK(pthread_cond_wait(&pcond_, mutex_.getPthreadMutex())); /* 等待Mutex */
	}
	void notify()
	{
		MCHECK(pthread_cond_signal(&pcond_)); /* 唤醒一个线程 */
	}
	void notifyAll()
	{
		MCHECK(pthread_cond_broadcast(&pcond_)); /* 唤醒多个线程 */
	}
private:
	MutexLock& mutex_;
	pthread_cond_t pcond_;
};

#endif
