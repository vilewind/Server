/**
 * @file server.cpp
 * @author vilewind
 * @brief UDP和TCPsocket可以同时绑定在一个端口，并处理事件，可以通过根据fd是否为对应的socket来判定相应的行为，同时也可以处理合适的事件
 * @version 0.1
 * @date 2022-05-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <cstdlib>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <errno.h>

using namespace std;

const static int MAX_EVENT_NUMBER = 1024;
const static int TCP_BUFFER_SIZE = 1024;
const static int UDP_BUFFER_SIZE = 1024;

int setNonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);

	return old_flag;
}

void addFd(int epoll_fd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	setNonblock(fd);
}

int main() {
	struct sockaddr_in addr;
	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5901);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
/*TCP socket,并绑定到对应的端口*/
	int tcp_fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(tcp_fd >= 0);
	int ret = bind(tcp_fd, (struct sockaddr*)&addr, sizeof(addr)));
	assert(ret != -1);
	ret = listen(tcp_fd, 5);
	assert(ret != -1);
/*UDP socket, 绑定到与tcp socket相同的端口 */
	bzero(&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
	addr.sin_port = htons(5901);
	int udp_fd = socket(PF_INET, SOCK_DGRAM, 0);
	assert(udp_fd >= 0);
/*epoll 创建epoll*/
	epoll_event events[MAX_EVENT_NUMBER];
	int epoll_fd = epoll_create(5);
	assert(epoll_fd >= 0);
/*注册 TCP和UDP上的可读事件*/
	addFd(epoll_fd, tcp_fd);
	addFd(epoll_fd, udp_fd);

	while(1) {
		int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
		if (number < 0) {
			cout << "epoll failure" << endl;
			break;
		}

		for (int i = 0; i < number; ++i) {
			int sock_fd = events[i].data.fd;
		/*TCP 监听有新连接，加入新连接的读事件到epoll*/
			if (sock_fd == tcp_fd) {
				struct sockaddr_in cli_addr;
				bzero(&cli_addr, sizeof(cli_addr));
				socklen_t len = sizeof(cli_addr);
				int cli_fd = accept(sock_fd, (struct sockaddr *)&cli_addr, &len);
				addFd(epoll_fd, cli_fd);
			} else if (sock_fd == udp_fd) {
				char buf[UDP_BUFFER_SIZE];
				memset(buf, 0, sizeof(buf));
				struct sockaddr_in addr;
				socklen_t len = sizeof(cli_addr);

				ret = recvfrom(sock_fd, buf, UDP_BUFFER_SIZE, 0, (struct sockaddr *)&cli_addr, &len);
				if (ret > 0) {
					sendto(sock_fd, buf, UDP_BUFFER_SIZE, 0, (struct sockaddr *)cli_addr, len);
				}
			} else if (events[i].events& EPOLLIN) {
				char buf[TCP_BUFFER_SIZE];
				while(1) {
					memset(buf, '\0', sizeof(buf));
					ret = recvfrom(sock_fd, buf, TCP_BUFFER_SIZE - 1, 0);
					
					if (ret < 0) {
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							break;
						}
						close(sock_fd);
					} else if (ret == 0) {
						close(sock_fd);
					} else {
						send(sock_fd, buf, ret, 0);
					}
				}
			} else {
				cout << "something else happened" << endl;
			}
		}
	}

	close(tcp_fd);
	return 0;
}