#include "general_header.h"

const static int TIMEOUT = 5000;

int timeout = TIMEOUT;
time_t start = time(nullptr);
time_t end = time(nullptr);
while(1) {
	std::cout << "the timeout is now " << timeout << " mil-seconds" << std::endl;
	start = time(nullptr);
	int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, timeout);
	if ((number < 0) && (errno != EINTR)) {
		std::cout << "epoll failure" << std::endl;
		break;
	}
/**
 * @attention 
 *  若epoll_wait返回零，说明超时时间到，此时处理定时任务，并重置定时时间
 * */
	else if (number == 0) {
		//处理定时任务
		timeout = TIMEOUT;
		continue;
	}
/**
 * @attention 若我epoll_wait返回值大于0，则本次epoll_wait持续时间为(end - start) * 1000ms，需要从定时器中减去这个时间段，以便获取下次epoll_wait调用的超时参数
 * 
 */
	end = time(nullptr);
	timeout -= (end - start) * 1000;
/**
 * @attention 重新计算的timeout值可能小于等于0，说明本次epoll_wait调用返回时，不仅有文件描述符号就绪，并且其他超时时间刚好到达，此时应当处理定时任务，并重置定时时间
 * 
 */
	if (timeout <= 0) {
		//处理定时任务
		timeout = TIMEOUT;
	}

	//handle connections
}