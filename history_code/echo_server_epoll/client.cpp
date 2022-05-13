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

	char send_buf[1024];
	char recv_buf[1024];
	while(1) {
		bzero(send_buf, sizeof(send_buf));
		bzero(recv_buf, sizeof(recv_buf));

		ssize_t in_size = read(STDIN_FILENO,send_buf, sizeof(send_buf)-1);
		if (strlen(send_buf)) {
			ssize_t send_size = func_except(write, serv_fd, send_buf, in_size);
		}
	/**
	 * @bug 发送完数据后，socket会一直卡在read的调用上
	 * 
	 */
		// ssize_t recv_size = read(serv_fd, recv_buf, sizeof(recv_buf)-1);
		// if (recv_size)
		// 	std::cout << recv_buf << std::endl;
	}

	close(serv_fd);

	return 0;
}