#include "Socket.h"
#include "Epoller.h"

using ChannelVec = std::vector<Channel *>;

int main() {
	Socket s("127.0.0.1", 88);

	int epoll_fd = epoll_create(5);

	Epoller poller;
	
	/* IO 监听*/
	Channel lis_channel{s.get_fd(), EPOLLIN | EPOLLET};
	poller.addEvent(lis_channel);
	setNonblock(s.get_fd());
//TODO IO监听
	while(1) {
		ChannelVec active_channels;
		poller.waitForEvent(active_channels);
		for (auto item : active_channels) {
			if (item->getFd() == s.get_fd()) {
				int cli_fd = s.Accept();
				Channel cli_channel(cli_fd, EPOLLIN | EPOLLET);
				poller.addEvent(cli_channel);
			} else if (item->getEvents() & EPOLLIN) {
				int cli_fd = item->getFd();
				int pipefd[2];
				pipe(pipefd);

				ssize_t ret = splice(cli_fd, nullptr, pipefd[1], nullptr, 35627, SPLICE_F_MORE | SPLICE_F_MOVE);
				assert(ret > -1);
				ret = splice(pipefd[0], nullptr, cli_fd, nullptr, 36527, SPLICE_F_MORE | SPLICE_F_MOVE);
			}
		}
	}
	return 0;
}