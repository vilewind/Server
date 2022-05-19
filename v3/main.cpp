#include "Epoller.h"
#include <vector>

using namespace std;

using ChannelVec = std::vector<Channel *>;

int main() {
	std::shared_ptr<Socket> s{new Socket("127.0.0.1", 88)};
	Epoller ep(s);
while(1) {
	ep.epollWait();
}
	return 0;
}