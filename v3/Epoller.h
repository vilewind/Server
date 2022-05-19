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
#include "Socket.h"
#include <sys/epoll.h>
#include <vector>
#include <memory>

class Channel;

class Epoller
{
public:
	using EventsVec = std::vector<epoll_event>;

public:
	Epoller(std::shared_ptr<Socket> ls):m_epfd(-1), m_listen_socket(ls) {
		m_epfd = funcException("epoll create", epoll_create, 256);
		
		m_ev.resize(MAX_EVENTS);
		m_ev.shrink_to_fit();

		addListenfd();
	}
	void addEvent(int fd) {
		epoll_event event;
		event.data.fd = fd;
		event.events = EPOLLIN | EPOLLET;
		int ret = funcException("epoll add", epoll_ctl, m_epfd, EPOLL_CTL_ADD, fd, &event);
		setNonblock(fd);
	}

	void modEvent(int fd) {
		epoll_event event;
		event.data.fd = fd;
		event.events = EPOLLOUT | EPOLLET;
		int ret = funcException("epoll add", epoll_ctl, m_epfd, EPOLL_CTL_MOD, fd, &event);
	}

	void epollWait(int timeout = -1) {
		int nfds = funcException("epoll wait", epoll_wait, m_epfd, (epoll_event *)&(*m_ev.begin()), m_ev.capacity(), timeout);

		for (int i = 0; i < nfds; ++i) {
			int sockfd = m_ev[i].data.fd;
			if (sockfd == m_listen_socket->getListenfd()) {
				int cli_fd = m_listen_socket->Accept();
				addEvent(cli_fd);
			} else if (m_ev[i].events & EPOLLIN) {
				char buf[1024];
				bzero(buf, sizeof(buf));

				int ret = recv(sockfd, buf, sizeof(buf), 0);
				if ( (ret < 0 && errno != EAGAIN) || ret == 0) {
					int ret = funcException("epoll remove", epoll_ctl, m_epfd, EPOLL_CTL_DEL, sockfd, nullptr);
					close(sockfd);
				} else {
					// modEvent(sockfd);
					send(sockfd, buf, sizeof(buf), 0);
				}
			} else if (m_ev[i].events & EPOLLOUT){
				// 
			} else {
				// 
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
