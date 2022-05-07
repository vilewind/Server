#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

int set_nonblock(int fd) {
/* 获取文件的旧状态标志*/	
	int old_flag = fcntl(fd, F_GETFL);
/* 设置非阻塞标志*/
	int new_flag = old_flag | O_NONBLOCK;
/* 设置新的文件描述副标志*/
	fcntl(fd, F_SETFL, new_flag);
/* 返回旧的文件描述符状态，以便恢复*/
	return old_flag;
}

int main () {
	struct sockaddr_in servaddr;
	struct sockaddr_in cliaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1",&servaddr.sin_addr);

	int servfd = socket(AF_INET, SOCK_STREAM, 0);
	int use = 1;
	// setsockopt(servfd, SOL_SOCKET, SO_REUSEADDR, &use, sizeof(use));
	bind(servfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	listen(servfd, 5);

	socklen_t len = sizeof(servaddr);
	int clifd = accept(servfd, (struct sockaddr *)&clifd, &len);

while(1) {
	char buf[1024];
	memset(buf, '\0', sizeof(buf));
	ssize_t ret = recv(clifd, buf, 1023, 0);
	printf("%d bytes data %s\n", ret, buf);

	memset(buf, '\0', sizeof(buf));
	ret = recv(clifd, buf, 1023,  MSG_OOB);
	printf("%d bytes data %s\n", ret, buf);

	memset(buf, '\0', sizeof(buf));
	ret = recv(clifd, buf, 1023, 0);
	printf("%d bytes data %s\n", ret, buf);
}
	close(servfd);
	close(clifd);
	return 0;
}