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
#include <string>

template<typename F, typename S, class... Args>
auto func_except(F&& f, S&& s, Args&& ... args)-> decltype(f(args...)) {
	using ret_type = decltype(f(args...));
	ret_type ret = f(args...);
	if (ret < 0) {
		throw std::runtime_error(std::string(s) + std::string(" error"));
	} else {
		return ret;
	}
}

int main() {
	struct sockaddr_in serv_addr;

	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(77);
	if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) < 0)
		throw std::runtime_error("Couldn't invert string to num");

	int serv_fd = func_except(socket, "socket", AF_INET, SOCK_STREAM, 0);

	int bind_flag = func_except(bind, "bind", serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	struct sockaddr_in cli_addr;
	socklen_t size = sizeof(cli_addr);
	bzero(&cli_addr, sizeof(cli_addr));

	int listen_flag = func_except(listen, "listen", serv_fd, 5);

	int cli_fd = func_except(accept, "accept", serv_fd, (struct sockaddr*)&cli_addr, &size);
while(1) {
	int pipefd_stdout[2];
	int pipe_flag = func_except(pipe, "pipe", pipefd_stdout);
/**
 * @brief 使用splice重定向文件描述符
 * @short ${1:short 重定向后的文件无法被读取
 */
/*< 将cli_fd获取的数据定向到管道中*/
	//int splice_flag = func_except(splice, cli_fd, nullptr, pipefd[1], nullptr, 32768, SPLICE_F_MORE|SPLICE_F_MOVE); 
/*< 将管道的输出定向到cli_fd*/
	//splice_flag = func_except(splice, pipefd[0], nullptr, cli_fd, nullptr, 32768, SPLICE_F_MORE|SPLICE_F_MOVE);

/**
 * @brief 使用tee实现echo服务器
 * 
 */
	int pipefd_file[2];
	pipe_flag = func_except(pipe, "pipe", pipefd_file);
/*< 将cli_fd输入管道pipefd_stdout中*/
	int splice_flag = func_except(splice, "splice", cli_fd, nullptr, pipefd_stdout[1], nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
/*< 将管道pipefd_stdout的输出复制到管道pipefd_file的输入端*/
	int tee_flag = func_except(tee, "tee", pipefd_stdout[0], pipefd_file[1], 32768, SPLICE_F_NONBLOCK);
/*< 将管道pipefd_file重定向到标准输出*/
	splice_flag = func_except(splice, "splice", pipefd_file[0], nullptr, STDOUT_FILENO, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
/*< 将管道pipefd_stdout重定向到clifd*/
	splice_flag = func_except(splice, "splice", pipefd_stdout[0], nullptr, cli_fd, nullptr, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
}
	close(cli_fd);
	close(serv_fd);

	return 0;
}