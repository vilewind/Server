#include <iostream>
#include <string>
#include <exception>

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

using namespace std;

// template<typename F, class... Args>
// struct processException: public std::exception{
// 	auto operator()(F&& f, Args&&... args)->std::decltype(f(args)) {
// 		auto item = 0;
// 		try
// 		{
// 			item = f(args);
// 			return item;
// 		}
// 		catch(const std::exception& e)
// 		{
// 			std::cerr << e.what() << '\n';
// 		}

// 		return -1;
// 	}
// };

int main(int argc, char** argv) {
	auto ip = "127.0.0.1";
	auto port = 80;

	struct sockaddr_in servaddr;
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	// servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	inet_pton(AF_INET, ip, &servaddr.sin_addr);

	int servfd = socket(AF_INET, SOCK_STREAM, 0);

	bind(servfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

	connect(servfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	
while(1) {
	std::string obb_data = "abc";
	std::string normal_data = "123";
	send(servfd, obb_data.c_str(), strlen(obb_data.c_str()), 0);

	send(servfd, normal_data.c_str(), strlen(normal_data.c_str()), MSG_OOB);

	send(servfd, obb_data.c_str(), strlen(obb_data.c_str()), 0);
}
	
	close(servfd);

	return 0;
}