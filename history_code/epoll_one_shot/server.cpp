/**
 * @file server.cpp
 * @author vilewind 
 * @brief 使用EPOLLONESHOT实现一个socket在任一时刻只能被一个线程处理，该线程处理完当前socket上的事务后，应重新注册事件，保证下一次EPOLLIN事件能够被触发。
 * @version 0.1
 * @date 2022-05-14
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <cstdlib>

#include <iostream>
#include <thread>

const static int MEX_EVENT_NUMBER = 1024;
const static int BUFFER_SIZE = 1024;

struct fds
{
	int epoll_fd;
	int sock_fd;
	fds(int fd1, int fd2) : epoll_fd(fd1), sock_fd(fd2){}
};

int setNonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);

	return old_flag;
}

void addFd(int epoll_fd, int fd, bool oneshot) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;

	if (oneshot) {
		event.events |= EPOLLONESHOT;
	}

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	setNonblock(fd);
}

/**
 * @brief 重置fd上的事件。尽管fd伤的EPOLLONESHOT事件被注册，但操作系统仍旧会触发EPOLLIN事件，且只触发一次
 * 
 * @param epoll_fd 
 * @param fd 
 */
void resetOneshot(int epoll_fd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;

	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &event);
}

void worker(void* arg) {
	int sock_fd = static_cast<fds*>(arg)->sock_fd;
	int epoll_fd = static_cast<fds*>(arg)->epoll_fd;

	std::cout << "start new thread to receive data on fd " << sock_fd << std::endl;

	char buf[1024];
	memset(buf, 0, sizeof(buf));

	while(1) {
		int ret = recv(sock_fd, buf, sizeof(buf) - 1, 0);
		if (ret == 0) {
			close(sock_fd);
			std::cout << "foreigner closed the connection" << std::endl;
			break;
		} else if (ret < 0) {
			/**
			 * @brief 
			 * 
			 */
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				resetOneshot(epoll_fd, sock_fd);
			}
		} else {
			std::cout << "get content : " << buf << std::endl;
			/**
			 * @brief 模拟数据处理过程
			 * 
			 */
			sleep(5);
		}
	}

	std::cout << "end thread receiving data on fd" << sock_fd << std::endl; 
}

int main(int argc, char *argv[]) {
	struct sockaddr_in sock_addr;
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(atoi(argv[1]));
	inet_pton(AF_INET, "127.0.0.1", &sock_addr.sin_addr);

	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(listen_fd >= 0);

	int bind_flag = bind(listen_fd, (struct sockaddr*)&sock_addr, sizeof(sock_addr));
	assert(bind_flag == 0);

	int listen_flag = listen(listen_fd, 5);
	assert(listen_flag >= 0);

	epoll_event events[1024];
	int epoll_fd = epoll_create(5);
	assert(epoll_fd >= 0);
	/**
	 * @brief 监听fd不能注册EPOLLONESHOT事件，否则应用程序只能处理一个客户链接，后续的客户链接请求将不再触发listenfd上的EPOLLIN事件
	 * 
	 */
	addFd(epoll_fd, listen_fd, false);

	while(1) {
		int ret = epoll_wait(epoll_fd, events, 1023, -1);
		if (ret < 0) {
			std::cout << "epoll failure" << std::endl;
			break;
		}

		for (int i = 0; i < ret; ++i) {
			int sock_fd = events[i].data.fd;
			if (sock_fd == listen_fd) {
				sockaddr_in cli_addr;
				socklen_t len = sizeof(cli_addr);
				int cli_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &len);
				assert(cli_fd >= 0);

				addFd(epoll_fd, cli_fd, true);
			} else if (events[i].events&EPOLLIN) {
				fds fd(epoll_fd, sock_fd);
				std::thread t(worker, (void *)&fd);
				t.detach();
			} else {
				std::cout << "something else happened" << std::endl;
			}
		}
	}

	close(listen_fd);
	return 0;
}