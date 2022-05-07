#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

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