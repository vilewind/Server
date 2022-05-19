/**
 * @file Event.h
 * @author vilewind 
 * @brief 封装epoll处理事件过程
 * @version 0.1
 * @date 2022-05-18
 * 
 * @copyright Copyright (c) 2022
 * 
 */
/**
 * @addindex 由于epoll_event的data部分是一个联合体，除了可以用来存储fd外，还有void*指针，因此可以通过自定义对象的方式来存储更多信息
*/
// typedef union epoll_data {
//                void        *ptr;
//                int          fd;
//                uint32_t     u32;
//                uint64_t     u64;
//            } epoll_data_t;

// struct epoll_event {
// 	uint32_t     events;      /* Epoll events */
// 	epoll_data_t data;        /* User data variable */
// };
/**
 * @addindex events types
 * 1. EPOLLIN 读事件
 * 2. EPOLLOUT 写事件
 * 3. EPOLLRDHUP   对端关闭（半关闭）
 * 4. EPOLLPRI	 高优先级可读，如TCP带外数据
 * 5. EPOLLET 边缘触发
 * 6. EPOLLONESHOT 保证一个socket连接在一定时间内只由一个线程操作
 */

#ifndef __EPOLLER_H__
#define __EPOLLER_H__

#include <iostream>
#include <functional>
#include <cstdint>
#include <cassert>
#include <vector>
#include <mutex>

#include <sys/epoll.h>

/**
 * @brief 封装epoll_event.data.ptr指向的对象，主要包括对应的sockfd、可能发生的事件及处理方式（函数），处理事件
 * 
 */
class Channel
{
public:
	using CbFunc = std::function<void()>;
public:
	// Channel() = default;
	Channel(int fd, uint32_t events) : m_fd(fd), m_events(events) { assert(fd >= 0); }
	~Channel() = default;

	void setFd(const int fd) { m_fd = fd; }
	int getFd() const { return m_fd; }

	void setEvents(const uint32_t event) { m_events = event; }
	uint32_t getEvents() const { return m_events; }

	void setReadCb(const CbFunc &cb) { m_readcb = cb; }
	void setWriteCb(const CbFunc &cb) { m_writecb = cb; }
	void setErrorCb(const CbFunc &cb) { m_errorcb = cb; }
	void setCloseCb(const CbFunc &cb) { m_closecb = cb; }

	void dealEvents() {
		if (m_events & EPOLLRDHUP) {
			std::cerr << "对方关闭 EPOLLRDHUP";
			m_closecb();
		} else if (m_events & (EPOLLIN | EPOLLPRI)) {
			std::cout << "读事件" << std::endl;
			m_readcb();
		} else if (m_events & EPOLLOUT) {
			std::cout << "写事件" << std::endl;
			m_writecb();
		} else {
			std::cerr << "出错";
			m_closecb();
		}
	}
private:
	int m_fd;
	uint32_t m_events;

	CbFunc m_readcb;
	CbFunc m_writecb;
	CbFunc m_errorcb;
	CbFunc m_closecb;
};

class Epoller
{
public:
	using EventVec = std::vector<epoll_event>;
	using ChannelVec = std::vector<Channel *>;

public:
	Epoller();
	~Epoller();

	void addEvent(const Channel &c);
	void modEvent(const Channel &c);
	void removeEvent(const Channel &c);
	void waitForEvent(ChannelVec &cv, int timeout = 0);

private:
	int m_epollfd;
	const static int MAX_EVENT_NUMBER = 1024;
	EventVec m_events;
	std::mutex m_mutex;
};

#endif
