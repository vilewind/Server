/**
 * @file server.cpp
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
#include <signal.h>
#include <errno.h>
#include <cstdio>

#include <vector>
#include <algorithm>

using namespace std;

int connFd(int listen_fd, fd_set& read_fds) {
	struct sockaddr_in cli_addr;
	bzero(&cli_addr, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(5901);
	inet_pton(AF_INET, "127.0.0.1", &cli_addr.sin_addr);
	socklen_t len = sizeof(cli_addr);
	int cli_fd = accept(listen_fd, (struct sockaddr *)&cli_addr, &len);

	FD_SET(cli_fd, &read_fds);
	// FD_SET(cli_fd, &write_fds);

	return cli_fd;
}

void sighandlerFunction(int signum, siginfo_t *siginfo, void* others) {
	if (signum == SIGPIPE) {
		cout << "client " << siginfo->si_fd << " closed" << endl;
		close(siginfo->si_fd);
	}
}

int main() {
	struct sockaddr_in serv_addr;
	bzero(&serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(5960);
	inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

	int listen_fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listen_fd >= 0);
	// int opt = 1;
	// if(setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) != 0)) {
	// 	cout << strerror(errno) << " reuse fail" << endl;
	// 	return -1;
	// }

	if( bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) != 0) {
		cout << strerror(errno) << " bind failed" << endl;
	}

	int flag = listen(listen_fd, 5);

	fd_set read_fds;
	// fd_set write_fds;
	FD_ZERO(&read_fds);
	// FD_ZERO(&write_fds);

	// FD_SET(STDOUT_FILENO, &read_fds);
	FD_SET(listen_fd, &read_fds);
	// FD_SET(listen_fd, &write_fds);

	vector<int> fd_vec(FD_SETSIZE, -1);
	// fd_vec[0] = STDIN_FILENO;
	// fd_vec[0] = listen_fd;
	int max_cur = -1;
	int tmp_max_cur = -1;
	int max_fd = listen_fd;

	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = sighandlerFunction;
	while (1) {
		fd_set all_read_fds = read_fds;
		// fd_set all_write_fds = write_fds;

		flag = select(max_fd + 1, &all_read_fds, nullptr, nullptr, 0);
		if (flag < 0)
			break;
		else if(flag == 0) {
			cout << " no events happen" << endl;
			continue;
		} else {
			if (FD_ISSET(listen_fd, &all_read_fds)) {
					int cli_fd = connFd(listen_fd, read_fds);
					auto iter = find(fd_vec.begin(), fd_vec.end(), -1);
					if (iter == fd_vec.end()) {
						cout << "too many connections, close the new connection " << cli_fd << endl;

						close(cli_fd);
					} else {
						int cur = distance(fd_vec.begin(), iter);
						fd_vec[cur - 1] = cli_fd;
						tmp_max_cur = max(max_cur, cur - 1);
						max_fd = max(max_fd, cli_fd);
						cout << "new connection " << cli_fd << endl;
					}
					--flag;
				}
			for (int i = 0; i <= max_cur && flag > 0; ++i) {
				int sock_fd = fd_vec[i];
				if (sock_fd < 0)
					continue;
				else if (FD_ISSET(sock_fd, &all_read_fds)) {
					// int local_fds[2];
					// int remote_fds[2];
					// flag = pipe(local_fds);
					// assert(flag >= 0);
					// flag = pipe(remote_fds);
					// assert(flag >= 0);

					// flag = splice(sock_fd, nullptr, local_fds[1], nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
					// assert(flag >= 0);
					// flag = tee(local_fds[0], remote_fds[1], 32768, SPLICE_F_NONBLOCK);
					// assert(flag >= 0);
					// flag = splice(local_fds[0], nullptr, sock_fd, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
					// assert(flag >= 0);
					// flag = splice(remote_fds[0], nullptr, sock_fd, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);

					// if (sigaction(SIGPIPE, &act, nullptr) < 0) {
					// 	cerr << "signal solve error" << endl;
					// } else {
					// 	FD_CLR(sock_fd, &read_fds);
					// 	fd_vec[i] = -1;
					// }

					char buf[1024];
					bzero(buf, sizeof(buf));
					int ret = recv(sock_fd, buf, sizeof(buf) - 1, 0);
					if (ret <= 0) {
						FD_CLR(sock_fd, &read_fds);
						close(sock_fd);
						fd_vec[i] = -1;
					} 
					send(sock_fd, buf, sizeof(buf), 0);
					--flag;
				}
			}
		}
	}

	close(listen_fd);

	return 0;
}
