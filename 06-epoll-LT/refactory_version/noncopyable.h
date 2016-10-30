#ifndef _NON_COPYABLE_H_
#define _NON_COPYABLE_H_

class noncopyable
{
protected:
	noncopyable() {} /* 这里主要是允许派生类对象的构造和析构 */
	~noncopyable() {}
private:
	noncopyable(const noncopyable&); /* 拷贝构造函数 */
	const noncopyable& operator=(const noncopyable&); /* 赋值操作符 */
};

#endif /* _NON_COPUABLE_H_ */
