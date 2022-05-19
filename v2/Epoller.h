#pragma once
#ifndef __EPOLLER_H__
#define __EPOLLER_H__
/**
 * @file Epoller.h
 * @author vilewind 
 * @brief 建立一个epoll红黑树，用于监听处理对应的事物
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
//TODO Channel指针管理
#include "Channel.h"
#include "Socket.h"
#include <sys/epoll.h>
#include <vector>
#include <memory>

class Channel;

class Epoller
{
public:
	using EventsVec = std::vector<epoll_event>;
	using ChannelVec = std::vector<std::shared_ptr<Channel>>;

public:
	Epoller(std::shared_ptr<Socket> ls):m_epfd(-1), m_listen_socket(ls) {
		m_epfd = funcException("epoll create", epoll_create, 256);
		
		m_ev.resize(MAX_EVENTS);
		m_ev.shrink_to_fit();

		addListenfd();
	}
/**
 * @vilewind 暴露智能指针的裸指针是否会影响智能指针的声明周期
 * 
 * @param channel 
 */
	void addEvent(Channel* channel) {
		epoll_event event;
		event.events = channel->getEvents();
		int fd = channel->getFd();
		event.data.ptr = channel;
		int ret = funcException("epoll add IO", epoll_ctl, m_epfd, EPOLL_CTL_ADD, fd, &event);
		setNonblock(fd);
	}

	void addEvent(int fd, uint32_t events) {
		Channel* channel(new Channel(fd, events));
		addEvent(channel);
	}

	// void removeEvent(int fd) {

	// }

	void epollWait(ChannelVec& cv, int timeout = -1) {
		int nfds = funcException("epoll wait", epoll_wait, m_epfd, (epoll_event *)&(*m_ev.begin()), MAX_EVENTS, timeout);

		for (int i = 0; i < nfds; ++i) {
		/* 监听socket*/	
			if (!m_ev[i].data.ptr && m_ev[i].data.fd == m_listen_socket->getListenfd()) {
				int cli_fd = m_listen_socket->Accept();
				addEvent(cli_fd, EPOLLIN | EPOLLET);
			} else {
				cv.emplace_back(m_ev[i].data.ptr);
			}

		}
	}
private:
	/**
 * @brief 将监听fd和监听事件添加到epoll内核事件表中
 * 
 * @param fd 监听fd
 */
	void addListenfd() {
		int fd = m_listen_socket->getListenfd();
		epoll_event event;
		event.data.fd = fd;
		event.events = EPOLLIN | EPOLLET;

		int ret = funcException("epoll add listenfd", epoll_ctl, m_epfd, EPOLL_CTL_ADD, fd, &event);
		setNonblock(fd);
	}

private:
	int m_epfd;
	std::shared_ptr<Socket> m_listen_socket;
	EventsVec m_ev;

	const static int MAX_EVENTS = 1024;
};

#endif
