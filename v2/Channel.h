#pragma once
#ifndef __CHANNEL_H__
#define __CHANNEL_H__

/**
 * @file Channel.h
 * @author vilewind 
 * @brief 绑定fd与可能的事件及处理事件的函数，对应于epoll_event.data.ptr
 * @version 0.1
 * @date 2022-05-19
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

#include "general_header.h"
#include "Epoller.h"
#include <functional>

class Epoller;

class Channel 
{
public:
	using Callback_Func = std::function<void()>;

public:
	Channel() = default;
	Channel(int fd, uint32_t events) : m_fd(fd), m_events(events){}
	~Channel() = default;

	inline void setFd(const int fd) { m_fd = fd; }
	inline int getFd() const { return m_fd; }

	inline void setEvents(const uint32_t events) { m_events = events; }
	inline uint32_t getEvents() const { return m_events; }

	inline void setReadCbFunc(Callback_Func &cbf) { m_read_cbf = cbf; }
	inline void setWriteCbFunc(Callback_Func &cbf) { m_write_cbf = cbf; }
	inline void setErrorCbFunc(Callback_Func &cbf) { m_error_cbf = cbf; }
	inline void setCloseCb(Callback_Func &cbf) { m_close_cbf = cbf; }

/**
 * @addindex events types
 * 1. EPOLLIN 读事件
 * 2. EPOLLOUT 写事件
 * 3. EPOLLRDHUP   对端关闭（半关闭）
 * 4. EPOLLPRI	 高优先级可读，如TCP带外数据
 * 5. EPOLLET 边缘触发
 * 6. EPOLLONESHOT 保证一个socket连接在一定时间内只由一个线程操作
 */
	void handleEvents() {
		if (m_events & EPOLLRDHUP) {
			std::cerr << "对方关闭 EPOLLRDHUP";
			m_close_cbf();
		} else if (m_events & (EPOLLIN | EPOLLPRI)) {
			std::cout << "读事件" << std::endl;
			m_read_cbf();
		} else if (m_events & EPOLLOUT) {
			std::cout << "写事件" << std::endl;
			m_write_cbf();
		} else {
			std::cerr << "出错";
			m_close_cbf();
		}
	}

private:
	int m_fd;
	uint32_t m_events;
	std::shared_ptr<Epoller> m_epoller;

	Callback_Func m_read_cbf;
	Callback_Func m_write_cbf;
	Callback_Func m_error_cbf;
	Callback_Func m_close_cbf;
};

#endif