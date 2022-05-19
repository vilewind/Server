#pragma once
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

/**
 * @brief 信号处理函数
 * 
 * @param sig 信号
 * @param fd 用于传递信息的socket，可以是管道，本项目采用eventfd
 */
void sigHandler(int sig, int fd) {
	int save_errno = errno;
	int msg = sig;
	send(fd, (char *)&msg, 1, 0);
	errno = save_errno;
}
/**
 * @brief 绑定信号处理函数
 * 
 * @param sig 
 * @param handler 
 * @param restart 
 */
void addSig(int sig, void(handler)(int), bool restart = true) {
	struct sigaction sa;
	bzero(&sa, sizeof(sa));
	if (restart)
		sa.sa_flags |= SA_RESTART;
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, nullptr) != -1);
}

/**
 * @brief 错误处理函数，用于释放可能的错误
 * 
 * @tparam Func 函数
 * @tparam Args 函数参数
 * @tparam S 函数名
 * @param f 
 * @param args 
 * @param s 
 */
template<typename Func, class... Args, class S>
auto funcException(S &s, Func& f, Args&& ... args)->decltype(f(args...)) {
	int ret = f(args);
	if (ret == -1){
		std::cerr << s << " failed";
		exit(EXIT_FAILURE);
	}
	// std::cout << s << " success" << std::endl;
	return ret;
}
#endif
