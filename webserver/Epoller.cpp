#include "Epoller.h"
#include "general_header.h"


Epoller::Epoller() {
	m_epollfd = epoll_create(1024);
	assert(m_epollfd >= 0);
	m_events.resize(MAX_EVENT_NUMBER);
	m_events.shrink_to_fit();
}
Epoller::~Epoller() {
	m_events.clear();
	m_events.shrink_to_fit();
	close(m_epollfd); //unistd.h
}

void Epoller::addEvent(const Channel &c) {
	int fd = c.getFd();
	epoll_event event;
	event.events = c.getEvents();
/**
 * @attention the type in a const_cast must be a pointer, reference, or pointer to member to an object type
 * auto channel = const_cast<Channel>(c);
 */
	auto channel = const_cast<Channel &>(c);
	event.data.ptr = static_cast<void *>(&channel);

	if(epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event) == -1) {
		std::cerr << "Failed to add in epoll";
		exit(-1);
	}
	setNonblock(fd);
	std::cout << "Successfully added in epoll" << std::endl;
}

void Epoller::modEvent(const Channel &c) {
	int fd = c.getFd();
	epoll_event event;
	event.events = c.getEvents();
	auto channel = const_cast<Channel &>(c);
	event.data.ptr = static_cast<void*>(&channel);

	if (epoll_ctl(m_epollfd, EPOLL_CTL_MOD, fd, &event) == -1) {
		std::cerr << "Failed to mod in epoll";
		exit(-1);
	}
}

void Epoller::removeEvent(const Channel &c) {
	int fd = c.getFd();
	if (epoll_ctl(m_epollfd, EPOLL_CTL_DEL, fd, 0) == -1) {
		std::cerr << "Failed to del in epoll";
		exit(-1);
	}
}

void Epoller::waitForEvent(ChannelVec &cv, int timeout) {
	int event_num = epoll_wait(m_epollfd, &(*m_events.begin()), m_events.capacity(), timeout);
	if (event_num == -1) {
		std::cerr << "Failed to wait in epoll, errno is " << errno;
		if (errno != EINTR)
			return;
			// exit(-1);
	}

	for (int i = 0; i < event_num; ++i) {
		cv.emplace_back(static_cast<Channel*>(m_events[i].data.ptr));
	}
	if (event_num == m_events.capacity()) {
		m_events.resize(2 * m_events.capacity());
		m_events.shrink_to_fit();
	}
}

//TODO 注册事件