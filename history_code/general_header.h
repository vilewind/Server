#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <exception>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <errno.h>

/**@brief 使用模版和parameters pack实现socketAPI调用失败意外
 * */
template<typename F, typename S, class... Args>
auto func_except(F&& f, Arg&& ...args)->decltype(f(args...)) {
	using RET_TYPE = decltype(f(args...));
	RET_TYPE ret = f(args...);

	if (ret < 0 ) {
		throw std::runtime_error(std::string(s) + std::string(" error");
	} else {
		return ret;
	}
}

/**@brief 使用fcntl汗函数将socket设置为非阻塞的
 * */
int set_nonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);
    /* 返回就的标志，便于修改*/	
	return old_flag;
}

/**@brief 将文件描述符fd上得EPOLL注册到epoll_fd指示的epoll内核事件表中
 * */
void add_fd(int epoll_fd, int fd, bool enable_et) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN;

	if (enable_et) 
		event.events |= EPOLLET;

	epoll_ctl(epollfd, EPOLL_CTL_ADD, &event);
	set_nonblock(fd);
}

void flow_by_level_trigger(epoll_event* events, int number, int epoll_fd, int listen_fd) {
	char buf[1024];
	for (int i = 0; i < number; ++i) {
		int fd = events[i].data.fd;
		if (fd == listen_fd) {
			struct sockaddr_in cli_addr;
			bzero(&cli_addr, sizeof(cli_addr));

			socklen_t len = sizeof(cli_addr);
			int cli_fd = func_except(accept, fd, (struct sockaddr*)&cli_addr, &len);
			addfd(epoll_fd, cli_fd, false);
		} else if(events[i].events&EPOLLIN) {
			std::cout << "event trigger once" << std::endl;
			bzero(buf, sizeof(buf));

			int ret = func_except(recv, fd, buf, sizeof(buf) - 1);
			std::cout << "get " << ret << " bytes of content:" << buf << std::endl; 
		}
	}
}

void flow_by_edge_trigger(epoll_event* event, int number, int epoll_fd, int listen_fd) {
	char buf[1024];
	for (int i = 0; i < number; ++i) {
		int fd = events[i].data.fd;
		if (fd == listen_fd) {
			struct sockaddr_in cli_addr;
			bzero(&cli_addr, sizeof(cli_addr));
			socklen_t len = sizeof(cli_addr);
			int fd = func_except(accept, fd, (struct sockaddr*)&cli_addr, &len);
			add_fd(epoll_fd,fd, true);
		} else if (events[i].events&EPOLLIN) {
			std::cout << "event trigger once" << std::endl;

			while(1) {
				bzero(buf, sizeof(buf));
				int ret = recv(fd, buf, sizeof(buf)-1);
				if (ret < 0 ) {
					if ((errno==EAGAIN) || (errno==EWOULDBLOCK)) {
						std::cout << " read later" << std::endl;
						break;
					} 
					close(fd);
					break;
				} else if(ret == 0) {
					close(fd);
				} else {
					std::cout << "get " << ret << " bytes of content: " << std::endl;
				}
			}
		}
	}
}	
