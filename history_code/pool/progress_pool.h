/**
 * @file progress_pool.h
 * @author vilewind 
 * @brief 半同步/半异步模式，同步线程/进程用于处理客户逻辑（逻辑单元），异步线程用于处理I/O事件（I/O处理单元）。异步线程/进程监听到客户请求后，将其封装成请求对象并插入请求队列中。请求队列将会通知某个工作在同步模式的工作线程来读取并处理该请求对象。
 * @version 0.1
 * @date 2022-05-17
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#ifndef __PROCESS_POOL_H__
#define __PROCESS_POOL_H__

#include "../general_header.h"

#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/epoll.h>

/**
 * @brief 子进程，m_pid为目标子进程的PID，m_pipefd为父进程与子进程通信的管道
 * 
 */
class process
{
public:
	process():m_pid(-1){}
public:
	pid_t m_pid;
	int m_pipefd[2];
};

template<typename T>
class processPool
{
private:
	processPool(int listenfd, int process_number = 8);
public:
/*< singleton*/
	static processPool<T>* create_instance(int listenfd, int process_number = 8) {
		static processPool<T> instance{listenfd, process_number};
		return &instance;
	}
	~processPool();
/*< 启动进程池*/
	void run();
private:
	void setup_sig_pipe();
	void run_parent();
	void run_child();
private:
	static const int MAX_PROCESS_NUMBER = 16;
	static const int USER_PER_PROCESS = 65535;
	static const int MAX_EVENTS_NUMBER = 10000;
/*< 进程池中现存进程数量*/
	int m_process_number;
/* 子进程在池中的序号，从0开始*/
	int m_idx;
/* 每个进程存在一个epoll内核事件表*/
	int m_epoll_fd;
/* 监听socket*/
	int m_listen_fd;
/* 子进程通过m_stop确定是否停止运行*/
	int m_stop;
/* 保存子进程的描述信息*/
	process *m_sub_process;
};
/* 用于处理信号的管道，称为信号管道，实现统一事件源*/
static int sig_pipefd[2];

static int setNonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);

	return old_flag;
}

static void addFd(int epoll_fd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);
	setNonblock(fd);
}

static void removeFd(int epoll_fd, int fd) {
	epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
	close(fd);
}

static void sig_handler(int sig) {
	int save_errno = errno;
	int msg = sig;
	send(sig_pipefd[1], (char *)&msg, 1, 0);
	errno = save_errno;
}

static void addSig(int sig, void(handler)(int), bool restart = true) {
	struct sigaction sa;
	memset(&sa, '\0', sizeof(sa));
	sa.sa_handler = handler;
	if (restart) {
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset(&sa.sa_mask);
	assert(sigaction(sig, &sa, nullptr) != -1);
}
/***********************************************线程池成员函数*********************************************************************/
template<typename T>
processPool<T>::processPool(int listen_fd, int process_number):m_listen_fd(listen_fd), m_process_number(process_number), m_idx(-1),
	m_stop(false) {
	assert((process_number > 0) && (process_number <= MAX_PROCESS_NUMBER));
	m_sub_process = new process[process_number];
	assert(m_sub_process);

	for (int i = 0; i < process_number; ++i) {
		int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, m_sub_process[i].m_pipefd);
		assert(ret == 0);
	/**
	 * @brief fork在父进程中返回子进程id，子进程中返回零
	 * 
	 */
		m_sub_process[i].m_pid = fork();
		assert(m_sub_process >= 0);
		if (m_sub_process[i].m_pid > 0) {
			close(m_sub_process[i].m_pipefd[1]);
			continue;
		} else {
			close(m_sub_process[i].m_pipefd[0]);
			m_idx = i;
			break;
		}
	}
}

/**
 * @brief 统一事件源
 * 
 * @tparam T 
 */
template<typename T>
void processPool<T>::setup_sig_pipe() {
	m_epoll_fd = epoll_create(5);
	assert(m_epoll_fd != -1);
	int ret = socketpair(PF_UNIX, SOCK_STREAM, 0, sig_pipefd);
	assert(ret != -1);
	setNonblock(sig_pipefd[1]);
	addFd(m_epoll_fd, sig_pipefd[0]);

	addSig(SIGCHLD, sig_handler);
	addSig(SIGTERM, sig_handler);
	addSig(SIGINT, sig_handler);
	addSig(SIGPIPE, SIG_IGN);
}

/**
 * @brief 父进程idx为-1，子进程idx大于-1，根据idx运行相应的进程
 * 
 * @tparam T 
 */
template<typename T>
void processPool<T>::run() {
	if (m_idx != -1) {
		run_child();
		return;
	}
	run_parent();
}

template<typename T>
void processPool<T>::run_child() {
	setup_sig_pipe();
	int pipefd = m_sub_process[m_idx].m_pipefd[1];
	addFd(m_epoll_fd, pipefd);
	epoll_event events[MAX_EVENTS_NUMBER];
	T *users = new T[USER_PER_PROCESS];
	assert(users);
	int number = 0;
	int ret = -1;

	while(!m_stop) {
		number = epoll_wait(m_epoll_fd, events, MAX_EVENTS_NUMBER, -1);
		if ((number < 0) && (errno != EINTR)) {
			std::cout << " epoll failed" << std::endl;
			break;
		}

		for (int i = 0; i < number; ++i) {
			int sockfd = events[i].data.fd;
			if (sockfd == pipefd && (events[i].events & EPOLLIN)) {
				int client = 0;
				ret = recv(sockfd, (char *)&client, sizeof(client), 0);
				if ((ret < 0 && errno != EAGAIN) || ret == 0) {
					continue;
				} else {
					sockaddr_in cli_addr;
					socklen_t len = sizeof(cli_addr);
					int cli_fd = accept(sockfd, (struct sockaddr *)&cli_addr, &len);
					if (cli_fd < 0) {
						std::cout << "errno is " << errno << std::endl;
						continue;
					}

					addFd(m_epoll_fd, cli_fd);
				/**
				 * @attention 模版类T必须实现init方法，用来初始化一个客户连接
				 * 
				 */
					users[cli_fd].init(m_epoll_fd, cli_fd, cli_addr);
				}
			} 
			/* 处理子进程获取的信号*/
			else if (sockfd == sig_pipefd[0] & (events[i].events & EPOLLIN)) {
				int sig;
				char signals[1024];
				ret = recv(pipefd, signals, sizeof(signals), 0);
				if (ret <= 0)
					continue;
				else {
					for (int i = 0; i < ret; ++i) {
						switch(signals[i]) {
							case SIGCHLD:
								{
									pid_t pid;
									int stat;
									while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
										continue;
									}
									break;
								}
							case SIGTERM:
							case SIGINT:
								{
									m_stop = true;
									break;
								}
							default:
								break;
						}
					}
				}
			} else if (events[i].events & EPOLLIN) {
				users[sockfd].process();
			} else {
				continue;
			}
		}
	}

	delete[] users;
	users = nullptr;
	close(pipefd);
/**
 * @attention listen_fd应由其创建者关闭，即“对象由哪个函数创建，就应该由哪个函数销毁
 * 
 */
	// close(listen_fd);
	close(m_epoll_fd);
}

template<typename T>
void processPool<T>::run_parent() {
	setup_sig_pipe();
	addFd(m_epoll_fd, m_listen_fd);
	epoll_event events[MAX_EVENTS_NUMBER];
	int sub_process_counter = 0;
	int new_conn = 1;
	int number = 0;
	int ret = -1;
	while(!m_stop) {
		number = epoll_wait(m_epoll_fd, events, MAX_EVENTS_NUMBER, -1);
		if ((number < 0) || errno != EINTR) {
			std::cout << "epoll_wait failed: " << errno << std::endl;
			break;
		}

		for (int i = 0; i < number; ++i) {
			int sockfd = events[i].data.fd;
			if (sockfd == m_listen_fd) {
			/* 采用round robin方式将新连接分配个一个子进程处理*/
				int i = sub_process_counter;

				do {
					if(m_sub_process[i].m_pid != -1) {
						break;
					}
					i = (i + 1) % m_process_number;
				} while (i != sub_process_counter);

				if (m_sub_process[i].m_pid == -1) {
					m_stop = true;
					break;
				}

				sub_process_counter = (i + 1) % m_process_number;
				send(m_sub_process[i].m_pipefd[0], (char *)&new_conn, sizeof(new_conn), 0);

				std::cout << "send request to child " << i << std::endl;
			} else if ((sockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN)) {
				int sig;
				char signals[1024];
				ret = recv(sig_pipefd[0], signals, sizeof(signals), 0);
				if (ret <= 0)
					continue;
				for (int i = 0; i < ret; ++i) {
					switch(signals[i]) {
						case SIGCHLD:
							{
								pid_t pid;
								int stat;
								while((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
									for (int i = 0; i < m_process_number; ++i) {
										if (m_sub_process[i].m_pid == pid) {
											std::cout << "child " << i << " join" << std::endl;
											close(m_sub_process[i].m_pipefd[0]);
											m_sub_process[i].m_pid = -1;
										}
									}
								}
							/* 若所有子进程都退出了，那么父进程也应当退出*/
								m_stop = true;
								for (int i = 0; i < m_process_number; ++i) {
									if (m_sub_process[i].m_pid != -1) {
										m_stop = false;
									}
								}
								break;
							}
						case SIGTERM:

					/* 若父进程接收到终止信号，那么杀死所有子进程，并等待其全部结束*/
					/** 通知子进程结束的更好方法是向父、子进程间的通信管道发送特殊数据 */
						case SIGINT:
							{
								std::cout << "kill all the child now " << std::endl;
								for (int i = 0; i < m_process_number; ++i) {
									int pid = m_sub_process[i].m_pid;
									if (pid != -1) {
										kill(pid, SIGTERM);
									}
								}

								break;
							}
						default:
							break;
					}
				}
			} else {
				continue;
			 }
		}
	}
	// close(listen_fd);
	close(m_epoll_fd);
}

#endif 