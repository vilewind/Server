/**
 * @file nonblock_connect.cpp
 * @author vilewind
 * @brief connect出错时会出现errno（EINPROGRESS），这种错误常发生在对非阻塞的socket调用connect而连接未立即建立时。
 * 		这种情况下，可以使用select、poll等函数来监听这个连接失败socket上的可写事件，当select、poll等函数返回后，利用getsockopt来读取错误码并清除该socket上的错误。
 * 		若错误码为0，表示连接成功建立，否则连接失败。
 * 		而通过这种非阻塞方式可以实现发起多个连接并一起等待。
 * @attention 该方法存在一些移植性问题
 * 			1、非阻塞的socket可能导致connect一直失败
 * 		 	2、select对处于EINPROGRESS状态下的socket可能失效
 * 			3、对于出错的socket，getsockopt在不同的系统中返回值存在差异。
 * @version 0.1
 * @date 2022-05-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "general_header.h"

using namespace std;

/**
 * @brief 超时连接函数
 * 
 * @param ip 
 * @param port 
 * @param time 
 * @return int 成功返回已连接的socket，失败返回-1
 */
int unblock_connect(const char* ip, int port, int time) {
	int ret = 0;
	struct sockaddr_in serv_addr;
	bzero(&servaddr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	inet_pton(AF_INET, ip, &serv_addr.sin_addr);

	int serv_fd = socket(AF_INET, SOCK_STREAM, 0);
	int fd_opt = set_nonblock(serv_fd);

	ret = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
/*1 连接成功恢复serv_fd属性*/
	if (ret == 0) {
		cout << "connect with server immediately" << endl;
		fcntl(serv_fd, F_SETFL, fd_opt);
		return serv_fd;
	} else if (errno != EINPROGRESS) {
/*2 未立即建立连接，只有个errno时EINPROGRESS时表示连接仍在进行*/
		cout << "unblock connect not support\n"
			 << endl;
		return -1;
	}

	fd_set read_fds;
	fd_set write_fds;
	struct timeval timeout;
	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);

	time_out.tv_sec = time;
	time_out.tv_usec = 0;

	ret = select(serv_fd + 1, &read_fds, &write_fds, nullptr, &time_out);
	if (ret <= 0) {
		cout << "connection time out" << endl;
		close(serv_fd);
		return -1;
	}

	if (!FD_ISSET(serv_fd, &write_fds)) {
		cout << "no events on sockfd found" << endl;
		close(serv_fd);
		return -1;
	}

	int error = 0;
	socklen_t len = sizeof(error);
/*3 调用getsockopt获取错误并清除 */
	if (getsockopt(serv_fd, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
		cout << "get socket option failed" << endl;
		close(serv_fd);
		return -1;
	}
/*3 错误号不为零表示连接出错*/
	if (error != 0) {
		cout << "connect failed after select with the error: " << error << endl;
		close(serv_fd);
		return -1;
	}

	cout << "connection ready after select with the socket: " << serv_fd << endl;
	fcntl(serv_fd, F_SETFL, fd_opt);
	return serv_fd;
}

int main() {
	int sockfd = unblock_connect("127.0.0.1", 5901, 10);
	if (sockfd < 0) {
		return 1;
	}
	close(sockfd);
	return 0;
}