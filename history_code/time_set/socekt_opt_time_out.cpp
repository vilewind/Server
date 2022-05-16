#include "../general_header.h"

int timeout_connect(std::string ip, int port, int time) {
	int ret = 0;
	sockaddr_in addr;
	bzero(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

	int sock_fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(sock_fd >= 0);

	timeval timeout;
	timeout.tv_sec = time;
	timeout.tv_usec = 0;

	socklen_t len = sizeof(timeout);
	ret = setsockopt(sock_fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, len);
	assert(ret != -1);

	ret = connect(sock_fd, (sockaddr *)&addr, sizeof(addr));
	if (ret == -1) {
		if (errno == EINPROGRESS) {
			std::cout << "connection timeout, process timeout logic" << std::endl;
		} else {
			std::cout << "error occur when connecting to server" << std::endl;
		}

		return -1;
	}

	return sock_fd;
}

int main(int argc, char *argv[]) {
	if (argc >= 2) {
		int port = atoi(argv[1]);
		int sock_fd = timeout_connect("127.0.0.1", port, 10);
		if (sock_fd < 0)
			return 1;
		else
			return - 1;
	}

	return 0;
}