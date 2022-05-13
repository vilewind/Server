#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <exception>
#include <stdexcept>
#include <functional>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <sys/epoll.h>
#include <sys/types.h>

#include <iostream>

template<typename F, typename S, class... Args>
auto func_except(F&& f, S&& s, Args&& ... args)-> decltype(f(args...)) {
	using ret_type = decltype(f(args...));
	ret_type ret = f(args...);
	if (ret < 0) {
		throw std::runtime_error(std::string(s) + std::string(" error"));
	} else {
		return ret;
	}
}


int setNonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);

	return old_flag;
}

/**
 * @brief 将事件为EPOLLIN的fd添加到内核时间表中，并采用ET模式
 * 
 * @param epoll_fd 
 * @param fd 
 */
void addFd(int epoll_fd, int fd) {
	if (fd < 0) {
		std::cerr << "illegal fd" << std::endl;
		return;
	}
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;

	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	setNonblock(fd);
}

int acceptNewCli(int listen_fd) {
	struct sockaddr_in cli_addr;
	socklen_t len = sizeof(cli_addr);

	int cli_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &len);
	return cli_fd;
}


int main() {
	struct sockaddr_in serv_addr;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(78);
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) < 0)
		throw std::runtime_error("Couldn't invert string to num");

	int serv_fd = func_except(socket, "socket", AF_INET, SOCK_STREAM, 0);

	int reuse_opt = 1;
	int reuse_flag = func_except(setsockopt, "setsockopt", serv_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_opt, sizeof(reuse_flag));
	int bind_flag = func_except(bind, "bind", serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
/*< 创建epoll文件描述符*/
	auto epoll_fd = epoll_create(10);

	int listen_flag = func_except(listen, "listen", serv_fd, 5);
/*< 将监听的serv_fd的EPOLLIN事件加入到epoll的事件表中*/	
	addFd(epoll_fd, serv_fd);

	epoll_event events[5];

	char buf[1024];
	while (1)
	{
		int events_num = epoll_wait(epoll_fd, events, 5, 0);
		
		if(events_num < 0) {
		/*< 信号中断，继续*/
			if (errno == EINTR)
				continue;
		/*< 出错*/
			break;
		} else if(events_num == 0) {
		/*< 超时*/
			continue;
		}
		
		for (int i = 0; i < events_num; i++) {
			int sock_fd = events[i].data.fd;
			if (sock_fd == serv_fd) {
				addFd(epoll_fd, acceptNewCli(sock_fd));
			} else if (events[i].events * EPOLLIN) {
			/**
			 * @attention 对于ET模式来说，EPOLLIN事件只会触发一次，所以需要循环读取，确保将socket缓存中的数据读出
			 * 
			 */
				while(1) {
					bzero(buf, sizeof(buf));
					int ret = recv(sock_fd, buf, 1, 0);
					if (ret < 0) {
					/**
					 * @attention 对于非阻塞IO，EAGAIN和EWOULDBLOCK表示数据已经全部读取。此后，epoll可以再次触发sock_fd上的EPOLLIN事件。
					 *
					 */
						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
							std::cout << sock_fd << " read later" << std::endl;
						} else {
							close(sock_fd);
						}
						break;
					} else if (ret == 0) {
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, nullptr);
						close(sock_fd);
						break;
					} else {
						std::cout << "get " << ret << " bytes of content " << buf << " from " << sock_fd << std::endl;
						//再次给cli_tfd注册检测可写事件
                        // struct epoll_event ee;
						// ee.events = EPOLLIN | EPOLLOUT | EPOLLET;
						// ee.data.fd = sock_fd;
						// epoll_ctl(epoll_fd, EPOLL_CTL_MOD, sock_fd, &ee);
					}
				}
			} else {
				std::cout << "something else happened" << std::endl;
			}
		}
	}
	close(serv_fd);

	return 0;
}