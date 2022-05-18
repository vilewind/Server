/**
 * @file client.cpp
 * @author vilewind 
 * @brief 
 * @version 0.1
 * @date 2022-05-11
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <cstdio>

using namespace std;

int main(){
	struct sockaddr_in serv_addr;
	memset(&serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5960);
	int flag = inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);
	// assert(flag == 0);

	int conn_fd = socket(PF_INET, SOCK_STREAM, 0);
	// assert(conn_fd >= 0);

	flag = connect(conn_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	cout << strerror(errno) << " " << flag << endl;
	fd_set read_fds;
	fd_set write_fds;
	// fd_set exception_fds[1024];

	FD_ZERO(&read_fds);
	FD_ZERO(&write_fds);
	// FD_ZERO(exception_fds);

	char send_buf[1024];
	char recv_buf[1024];
	int local_fds[2];
	int remote_fds[2];
	flag = pipe(local_fds);
	// assert(flag == 0);
	flag = pipe(remote_fds);
	// assert(flag == 0);
	while(1) {
		FD_SET(conn_fd, &read_fds);
		// FD_SET(conn_fd, &write_fds);
		FD_SET(STDIN_FILENO, &read_fds);
		// FD_SET(conn_fd, &write_fds);

		flag = select(conn_fd + 1, &read_fds, &write_fds, nullptr, 0);
		// assert(flag >= 0);
		if (flag < 0)
			break;

/**
 * @attention pipe函数创建的文件描述符fd[0]和fd[1]分别构成管道的两端，往fd[1]写入的数据可以从fd[0]读出。
 * 			并且fd[1]只能用于写入数据，fd[0]只能用于读出数据
 * 
 */
	/*< 通过tee、splice和pipe实现零拷贝*/
		if (FD_ISSET(STDIN_FILENO, &read_fds)) {
			// flag = splice(STDIN_FILENO, nullptr, conn_fd, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			// assert(flag >= 0);
			// flag = tee(local_fds[0], remote_fds[1], 32768, SPLICE_F_NONBLOCK);
			// assert(flag >= 0);
			// flag = splice(remote_fds[0], nullptr, conn_fd, nullptr, 326768, SPLICE_F_MORE | SPLICE_F_MOVE);
			// assert(flag >= 0);
			// flag = splice(local_fds[0], nullptr, STDOUT_FILENO, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			char buf[1024];
			bzero(buf, sizeof(buf));
			recv(STDIN_FILENO, buf, sizeof(buf), 0);
			send(conn_fd, buf, sizeof(buf), 0);
		} 
	/*< 读取来自服务器的信息*/
		else if (FD_ISSET(conn_fd, &read_fds)) {
			bzero(recv_buf, sizeof(recv_buf));
			int ret = recv(conn_fd, recv_buf, sizeof(recv_buf) - 1, 0);
			if (ret < 0) {
				if (errno != EAGAIN || errno != EWOULDBLOCK) {
					cout << "failure" << endl;
				}
				break;
			} if (ret < 0) {
				break;
			} else {
				if (strncmp(recv_buf, "quit", 4) == 0){
					cout << "we will quit" << endl;
					break;
				}
			}
		}
	}

	close(conn_fd);

	return 0;
}