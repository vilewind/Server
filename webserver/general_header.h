#ifndef __GH__
#define __GH__

#include <iostream>
#include <string>
#include <cassert>

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
#include <signal.h>

#include <thread>

int setNonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	assert(old_flag >= 0);
	int new_flag = old_flag | O_NONBLOCK;
	int ret = fcntl(fd, F_SETFL, new_flag);
	assert(ret >= 0);
	return old_flag;
}

void addrReuse(int fd) {
	int on = 1;
	int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	if (ret < 0)
		std::cerr << errno << std::endl;
}
#endif
