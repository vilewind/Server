#include "../general_header.h"
#include "timeout_link.h"

const static int FD_LIMIT = 1024;
const static int MAX_EVENT_NUMBER = 1024;
const static int TIMESLOT = 5;

static int pipefd[2];

static sort_timer_list time_lst;
static int epoll_fd = 0;

int setNoblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);

	return old_flag;
}

void addEvent(int epoll_fd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	setNoblock(fd);
}

/**
 * @brief 
 * @todo 为什么在处理errno前要保存
 * 
 * @param sig 
 */
void sigHandler(int sig) {
	int save_errno = errno;
	int msg = sig;
	send(pipefd[1], (char *)&msg, 1, 0);
	errno = save_errno;
} 

void addSig(int sig) {
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = sigHandler;
	sa.sa_flags |= SA_RESTART;

	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, nullptr) != -1);
}

/**
 * @brief 调用tick函数，处理定时任务
 * 
 */
void timeHandler() {
	time_lst.tick();
/*<  调用一次alarm调用只会引起SIGALRM信号，因此需要重新定时，来不断触发SIGALRM信号*/
	alarm(TIMESLOT);
}

/**
 * @brief 定时器回调函数，处理不活跃的注册事件
 * 
 * @param cli_addr 
 */
void cbFunc(client_data* cli_data){
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cli_data->sock_fd, 0);
	assert(cli_data);
	close(cli_data->sock_fd);

	std::cout << " close fd " << cli_data->sock_fd << std::endl;
}

int main() {
	sockaddr_in lis_addr;
	bzero(&lis_addr, sizeof(lis_addr));
	lis_addr.sin_family = AF_INET;
	lis_addr.sin_port = htons(88);
	inet_pton(AF_INET, "127.0.0.1", &lis_addr.sin_addr);

	int lis_fd = socket(PF_INET, SOCK_STREAM, 0);
	assert(lis_fd >= 0);

	int bind_flag = bind(lis_fd, (struct sockaddr *)&lis_fd, sizeof(lis_fd));
	assert(bind_flag > -1);

	int lis_flag = listen(lis_fd, 5);
	assert(lis_flag > -1);

	int epoll_fd = epoll_create(5);
	assert(epoll_fd > -1);
	addEvent(epoll_fd, lis_fd);

	epoll_event events[1024];

	int sp_flag = socketpair(PF_UNIX, SOCK_STREAM, 0, pipefd);
	assert(sp_flag > -1);
	setNoblock(pipefd[1]);
	addEvent(epoll_fd, pipefd[0]);
/*< 设置信号处理函数*/
	addSig(SIGALRM);
	addSig(SIGTERM);

	bool stop_server = false;
	client_data *clients = new client_data[FD_LIMIT];
	bool timeout = false;
	alarm(TIMESLOT);

	while(!stop_server) {
		int number = epoll_wait(epoll_fd, events, MAX_EVENT_NUMBER, -1);
		if (number < 0 && errno != EINTR) {
			std::cout << " epoll failure " << std::endl;
			break;
		}

		for (int i = 0; i < number; ++i) {
			if (events[i].data.fd == lis_fd) {
				sockaddr_in cli_addr;
				socklen_t len = sizeof(cli_addr);

				int cli_fd = accept(lis_fd, (sockaddr *)&cli_addr, &len);
				assert(cli_fd >= 0);
				addEvent(epoll_fd, cli_fd);
			/*< 创建定时器，设置回调函数与超时时间，绑定定时与用户数据，而后加入到定时器链表中*/
				clients[cli_fd].addr = cli_addr;
				clients[cli_fd].sock_fd = cli_fd;
				util_timer *timer = new util_timer;
				timer->user_data = &clients[cli_fd];
				timer->cb_func = cbFunc;
				time_t cur = time(nullptr);
				timer->expire = cur + 3 * TIMESLOT;
				clients[cli_fd].timer = timer;
				time_lst.add_timer(timer);
			} 
		/*< 处理信号*/
			else if (events[i].data.fd == pipefd[0] && events[i].events & EPOLLIN){
				int sig;
				char signals[1024];
				int ret = recv(pipefd[0], signals, sizeof(signals), 0);
				if (ret == -1)	continue;
				else if (ret == 0)	continue;
				else {
					for (int i = 0; i < ret; ++i) {
						switch (signals[i])
						{
						case SIGALRM:
						/**
						 * @attention 使用timeout标记有定时任务需要处理，但并不立即处理该任务。因为定时任务的优先级并不高，要优先处理其他任务
						 */
							timeout = true;
							break;
						case SIGTERM:
							stop_server = true;
						// default:
						// 	break;
						}
					}
				}
			} else if (events[i].events & EPOLLIN) {
				memset(clients[i].buffer, '\0', BUFFER_SIZE);
				int ret = recv(events[i].data.fd, clients[i]. buffer, BUFFER_SIZE - 1, 0);
			
				if (ret < 0) {
					if (errno == EAGAIN || errno == EWOULDBLOCK) {
						std::cout << "out of time" << std::endl;
						cbFunc(&clients[events[i].data.fd]);
						if (clients[events[i].data.fd].timer) {
							time_lst.del_timer(clients[events[i].data.fd].timer);
						}
					}
				} else if (ret == 0) {
					cbFunc(&clients[events[i].data.fd]);
					close(events[i].data.fd);
					if (clients[events[i].data.fd].timer) {
							time_lst.del_timer(clients[events[i].data.fd].timer);
						}
				} else {
					std::cout << static_cast<std::string>(clients[i].buffer) << std::endl;
				}
			}

			if (timeout) {
				timeHandler();
				timeout = false;
			}
		}

		close(lis_fd);
		close(pipefd[0]);
		close(pipefd[1]);

		delete[] clients;

		return 0;
	}
}