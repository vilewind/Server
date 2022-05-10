#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <poll.h>
#include <vector>
#include <string>
#include <fcntl.h>

using namespace std;

int main() {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	int fd = socket(AF_INET, SOCK_STREAM, 0);

	// bind(fd, (struct sockaddr *)&addr, sizeof(addr));

	if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
		cout << "connection failed" << endl;
		close(fd);
		return 1;
	}

	pollfd fds[2];
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	fds[1].fd = fd;
	fds[1].events = POLLIN | POLLRDHUP;
	fds[1].revents = 0;

	char read_buf[1024];

	int pipe_fd[2];
	assert(pipe(pipe_fd) != -1);

	while(1) {
		int ret = poll(fds, 2, -1);
		if (ret < 0) {
			cout << "poll failure" << endl;
			continue;
		} else if(fds[1].revents & POLLRDHUP){
			cout << "server close the connection" << endl;
			break;
		} else if(fds[1].revents & POLLIN) {
			memset(read_buf, '\0', sizeof(read_buf));
			recv(fds[1].fd, read_buf, sizeof(read_buf)-1, 0);
			cout << to_string(read_buf) << endl;
		}

		if (fds[0].revents&EPOLLIN) {
			ret = splice(0, nullptr, pip_fd[1], nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
			ret = splice(pipe_fd[0], nullptr, fd, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
		}
	}

	close(fd);

	return 0;
}