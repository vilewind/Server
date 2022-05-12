#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <exception>
#include <stdexcept>
#include <functional>
#include <unistd.h>
#include <iostream>

template<typename F, class... Args>
auto func_except(F&& f, Args&& ... args)-> decltype(f(args...)) {
	using ret_type = decltype(f(args...));
	ret_type ret = f(args...);
	if (ret < 0) {
		throw std::runtime_error("error");
	} else {
		return ret;
	}
}

int main () {
	struct sockaddr_in serv_addr;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(78);
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) < 0)
		throw std::runtime_error("Couldn't invert string to num");

	int serv_fd = func_except(socket, AF_INET, SOCK_STREAM, 0);

	int bind_flag = bind(serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	int connect_flag = connect(serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	fd_set read_fds;
	FD_ZERO(&read_fds);
	FD_SET(STDIN_FILENO, &read_fds);
	FD_SET(serv_fd, &read_fds);

	char send_buf[1024];
	char recv_buf[1024];
	while(1) {
		FD_SET(STDIN_FILENO, &read_fds);
		FD_SET(serv_fd, &read_fds);

		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			bzero(send_buf, sizeof(send_buf));
			ssize_t in_size = read(STDIN_FILENO,send_buf, sizeof(send_buf)-1);
			if (in_size > 0) {
				ssize_t send_size = func_except(write, serv_fd, send_buf, sizeof(send_buf)-1);
			} 
		} 
		if(FD_ISSET(serv_fd, &read_fds)) {
			bzero(recv_buf, sizeof(recv_buf));
			ssize_t recv_size = read(serv_fd, recv_buf, sizeof(recv_buf)-1);
			if (recv_size)
				std::cout << recv_buf << std::endl;
			else if (recv_size <= 0) {
				break;
			}
		}
	}
	FD_CLR(serv_fd, &read_fds);
	FD_CLR(STDIN_FILENO, &read_fds);
	close(serv_fd);

	return 0;
}