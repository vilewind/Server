#include "progress_pool.h"

class cgi_conn 
{
public:
	cgi_conn();
	~cgi_conn();

	void init(int epoll_fd, int sockfd, const sockaddr_in &cli_addr) {
		m_epollfd = epoll_fd;
		m_sockfd = sockfd;
		m_addr = cli_addr;
		memset(m_buf, '\0', BUFFER_SIZE);
		m_read_idx = 0;
	}

	void process() {
		int idx = 0;
		int ret = -1;

		while(true) {
			idx = m_read_idx;
			ret = recv(m_sockfd, m_buf + idx, BUFFER_SIZE - 1 - idx, 0);
			if (ret < 0) {
				if (errno != EAGAIN) {
					removeFd(m_epollfd, m_sockfd);
				}
				break;
			} else if (ret == 0){
				removeFd(m_epollfd, m_sockfd);
				break;
			} else {
				m_read_idx += ret;
				std::cout << "user content is " << m_buf << std::endl;
			/* 若遇到字符“\r\n“，开始处理客户请求*/
				for (; idx < m_read_idx; ++idx) {
					if(idx >= 1 && m_buf[idx-1] == '\r' && m_buf[idx] == '\n') {
						break;
					}
				}
			/* 若没有遇到字符“\r\n”，则需要读取更多客户数据*/
				if (idx == m_read_idx) {
					continue;
				}
				m_buf[idx - 1] = '\0';
				char *file_name = m_buf;
			/* 判断客户要运行的CGI程序是否存在*/	
				if (access(file_name, F_OK) == -1) {
					removeFd(m_epollfd, m_sockfd);
					break;
				}

				ret = fork();
				if (ret == -1) {
					removeFd(m_epollfd, m_sockfd);
					break;
				} else if (ret > 0) {
				/* 父进程关闭连接*/
					removeFd(m_epollfd, m_sockfd);
					break;
				} else {
				/* 子进程将标准输出定向到m_sockfd，并执行CGI程序*/
					close(STDOUT_FILENO);
					dup(m_sockfd);
					execl(m_buf, m_buf, 0);
					exit(0);
				}
			}
		}
	}
private:
/* 读缓冲区的大小*/
	static const int BUFFER_SIZE = 1024;
	static int m_epollfd;
	int m_sockfd;
	sockaddr_in m_addr;
	char m_buf[BUFFER_SIZE];
/* 标记读缓冲区已经读入的客户数据的最后一个字节的下一个位置*/
	int m_read_idx;
};

int cgi_conn::m_epollfd = -1;

int main() {
	int lis_fd = socket(AF_INET, SOCK_STREAM, 0);
	assert(lis_fd >= 0);
	int ret = 0;
	sockaddr_in lis_addr;
	bzero(&lis_addr, sizeof(lis_addr));
	lis_addr.sin_family = AF_INET;
	lis_addr.sin_port = htons(88);
	inet_pton(AF_INET, "127.0.0.1", &lis_addr.sin_addr);

	ret = bind(lis_fd, (sockaddr *)&lis_addr, sizeof(lis_addr));
	assert(ret != -1);
	ret = listen(lis_fd, 5);
	assert(ret != -1);

	processPool<cgi_conn> *pool = processPool<cgi_conn>::create_instance(lis_fd);
	if (pool) {
		pool->run();
		delete pool;
	}
/**
 * @attention main函数创建lis_fd，那么应由main函数关闭
 * 
 */
	close(lis_fd);
	return 0;
}