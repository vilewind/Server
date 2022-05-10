#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <poll.h>
#include <vector>
#include <string>
#include <fcntl.h>

using namespace std;

const static size_t FD_LIMIT = 65535;

struct cli_data 
{
	sockaddr_in addr;
	string write_buf;
	char buf[1024];
};

int set_nonblock(int fd) {
	int old_flag = fcntl(fd, F_GETFL);
	int new_flag = old_flag | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flag);

	return old_flag;
}

int main() {
	struct sockaddr_int serv_addr;
	memset(&serv_addr, '\0', sizeof(serv_addr));
	

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(80);
	inet_pton(AF_INET, "127.0.0.1", &serv_add.sin_addr);

	int serv_fd = socket(AF_INET, SOCK_STREAM, 0);

	bind(serv_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

	listen(serv_fd, 5);

	cli_data *users = new cli_data[FD_LIMIT];
	// vector<cli_data *> clients(FD_LIMIT);

	pollfd fds[6];
	int user_counter = 0;

	for (int i = 1; i < 6; ++i) {
		fds[i].fd = -1;
		fds[i].events = 0;
	}
	fds[0].fd = serv_fd;
	fds[0].events = POLLIN | POLLERR;
	fds[0].revents = 0;

	while(1) {
		int ret = poll(fds, user_counter + 1, -1);
		if (ret < 0) {
			cout << "poll failure" << endl;
			break;
		}

		for (int i = 0; i < 6; ++i) {
			if ((fds[i].fd == serv_fd) && (fds[i].revents &POLLIN)) {
				struct sockaddr_in cli_addr;
				memset(&cli_addr, '\0', sizeof(cli_addr));
				size_t len = sizeof(cli_addr);
				int conn_fd = accept(fds[i].fd, (struct sockaddr *)*cli_addr, &len);
				if (conn_fd < 0) {
					cout << "errno is " << errno << endl;
					continue;
				}

				if (user_counter >= 6) {
					string str = "too many users";
					cout << str << endl;
					send(conn_fd, str.c_str(), str.size(), 0);
					close(conn_fd);
					continue;
				}

				++user_counter;
				users[conn_fd]->addr = cli_addr;
				users[conn_fd]->events = POLLIN | POLLRDHUP | POLLERR;
				users[conn_fd]->revents = 0;

				cout << "a new user " << conn_fd << ", here keeps " << user_counter << " users" << endl;
			} else if(fds[i].revents & POLLERR) {
				cout << "get an error from " << fds[i].fd << endl;
				char errors[100];
				memset(errors, 0, sizeof(errors));
				socklen_t len = sizeof(errors);
				if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &len) < 0){
					cout << "get socket option failed" << endl;
				}
				continue;
			} else if (fds[i].revents & POLLRDHUP) {
				users[fds[i].fd] = users[fds[user_counter].fd];
				close(fds[i].fd);

				fds[i] = fds[user_counter];
				--i;
				--user_counter;
			} else if(fds[i].revents & POLLIN) {
				int conn_fd = fds[i].id;
				memset(users[conn_fd].buf, '\0', 1024);
				ret = recv(conn_fd, fds[i].buf, 1023, 0);
				cout << "get " << ret << "bytes of client data " << users[conn_fd].buf << " from " << conn_fd << endl;
				if (ret < 0) {
					if (errno != EAGAIN) {
						close(conn_fd);
						users[fds[i].fd] = users[fds[user_counter].fd];
						fds[i] = fds[user_counter];
						--i;
						--user_counter;
					}
				} else if (ret == 0) {
					//..
				} else {
					for (int j = 1; j <= user_counter; ++j) {
						if (fds[j].fd == conn_fd) {
							continue;
						}
						fds[j].events |= ~POLLIN;
						fds[j].evnets |= POLLOUT;

						users[fds[j].fd]->write_buf = users[conn_fd].buf;
					}
				}
			}  else if (fds[i].revents & POLLOUT) {
				int conn_fd = fds[i].fd;
				if (!users[conn_fd]->write_buf) {
					continue;
				}

				ret = send(conn_fd, users[conn_fd].write_buf, strlen(users[conn_fd]->write_buf], 0);
				users[conn_fd]->write_buf = NULL;

				fds[i].events |= ~POLLOUT;
				fds[i].events |= POLLIN;
			}
		}
	}

	delete[] users;
	close(serv_fd);

	return 0;
}

