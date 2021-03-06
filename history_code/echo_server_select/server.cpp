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
#include <signal.h>

#include <iostream>
#include <vector>
#include <string>
#include <algorithm>

using std::cout;
using std::endl;
using std::string;
using std::vector;

template<typename F, typename S, class... Args>
auto func_except(F&& f, S&& s, Args&& ... args)-> decltype(f(args...)) {
	using ret_type = decltype(f(args...));
	ret_type ret = f(args...);
	if (ret < 0) {
		throw std::runtime_error(string(s) + string(" error"));
	} else {
		return ret;
	}
}

int new_connection(fd_set& fds, int sock_fd) {
	struct sockaddr_in cli_addr;
	bzero(&cli_addr, sizeof(cli_addr));
	socklen_t len = sizeof(cli_addr);
	int cli_fd = func_except(accept, "accept", sock_fd, (struct sockaddr *)&cli_addr, &len);
	cout << "create new connection " << cli_fd << endl;
	return cli_fd;
}

void sighadler_func(int signum, siginfo_t *siginfo, void* others) {
	if (signum == SIGPIPE) {
		cout << "client " << siginfo->si_fd << " closed" << endl;
	}
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

	int listen_flag = func_except(listen, "listen", serv_fd, 5);

	struct sigaction act;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = sighadler_func;

	fd_set read_fds;
	fd_set all_fds;
	FD_ZERO(&all_fds);
	FD_ZERO(&read_fds);
	vector<int> fd_vec(FD_SETSIZE, -1);
	int max_fd = serv_fd;
	int max_cur = -1;

	FD_SET(serv_fd, &read_fds);
	max_fd = serv_fd;

	while(1) {
		all_fds = read_fds;
		int prepared_fds = select(max_fd + 1, &all_fds, nullptr, nullptr, 0);

		if (FD_ISSET(serv_fd, &all_fds)) {
			int cli_fd = new_connection(read_fds, serv_fd);
			max_fd = std::max(cli_fd, max_fd);

			auto iter = std::find(fd_vec.begin(), fd_vec.end(), -1);
			if(iter == fd_vec.end()) {
				cout << "too many connection" << endl;
				close(cli_fd);
			} else {
				int cur = std::distance(fd_vec.begin(), iter);
				max_cur = std::max(max_cur, cur);
				max_fd = std::max(max_fd, cli_fd);
				fd_vec[cur] = cli_fd;
				FD_SET(cli_fd, &read_fds);
				cout << "new client " << cli_fd << endl; 
			}
			--prepared_fds;
		}

		for (int i = 0; i <= max_cur && prepared_fds > 0; ++i) {
			int cli_fd = fd_vec[i];
			if (cli_fd < 0)	continue;
			if(FD_ISSET(cli_fd, &all_fds)) {
			// 	int pipefd_stdout[2];
			// 	int pipe_flag = func_except(pipe, "pipe", pipefd_stdout);
			// 	int pipefd_file[2];
			// 	pipe_flag = func_except(pipe, "pipe", pipefd_file);

			// /*< ???cli_fd????????????pipefd_stdout???*/
			// 	int splice_flag = func_except(splice, "splice", cli_fd, nullptr, pipefd_stdout[1], nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			// /*< ?????????pipefd_stdout????????????????????????pipefd_file????????????*/
			// 	int tee_flag = func_except(tee, "tee", pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_NONBLOCK);
			// /*< ?????????pipefd_file????????????????????????*/
			// 	splice_flag = func_except(splice, "splice", pipefd_file[0], nullptr, STDOUT_FILENO, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			// /*< ?????????pipefd_stdout????????????clifd*/
			// 	splice_flag = func_except(splice, "splice", pipefd_stdout[0], nullptr, cli_fd, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);

			// 	if (sigaction(SIGPIPE, &act, nullptr) < 0) {
			// 		std::cerr << "signal solve error" << endl;
			// 	} else {
			// 		close(cli_fd);
			// 		FD_CLR(cli_fd, &read_fds);
			// 		fd_vec[i] = -1;
			// 	}

				char buf[1024];
				bzero(buf, sizeof(buf));
				int ret = recv(cli_fd, buf, sizeof(buf) - 1, 0);
				if (ret <= 0) {
					close(cli_fd);
					FD_CLR(cli_fd, &read_fds);
					fd_vec[i] = -1;
				} else {
					cout << string(buf) << endl;
					send(cli_fd, buf, ret, 0);
				}

				--prepared_fds;
			}
		}
	}

	close(serv_fd);

	return 0;
}