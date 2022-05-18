#ifndef __SOCKET_H__
#define __SOCKET_H__

#include "general_header.h"

#include <string>
#include <memory>

class Socket 
{
public:
	Socket(const std::string &host, const int port):m_addr(new sockaddr_in) {
		m_addr->sin_family = AF_INET;
		m_addr->sin_port = htons(port);
		int ret = inet_pton(AF_INET, host.c_str(), &m_addr->sin_addr);
		if (ret == 0) {
			std::cerr << "host does not contain a character string representing a valid network address in the specified address family." << std::endl;
			exit(-1);
		}
		if (ret < 0) {
			std::cerr << errno << std::endl;
			exit(-1);
		}
		m_fd = socket(PF_INET, SOCK_STREAM, 0);
		addrReuse(m_fd);
		assert(m_fd >= 0);
		Bind();
		Listen();
	}
	~Socket() {
		if (m_addr) {
			delete m_addr;
			m_addr = nullptr;
		}
		close(m_fd);
	}
public:
	int get_fd() const { return m_fd; }
	int Accept() {
		sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_fd = accept(m_fd, (sockaddr*)&cli_addr, &len);
		assert(cli_fd >= 0);
		std::cout << "accept a client " << cli_fd << std::endl;
		return cli_fd;
	}

private:
	void Bind() {
		if (bind(m_fd, (sockaddr*)m_addr, sizeof(sockaddr)) < 0){
			std::cerr << "bind error" << std::endl;
			exit(1);
		}
		std::cout << "bind" << std::endl;
	}
	void Listen(){
		if (listen(m_fd, 10) < 0) {
			std::cerr << "listen error" << std::endl;
			exit(1);
		}
		std::cout << "listening" << std::endl;
	}

private:
	int m_fd;
	sockaddr_in* m_addr;
};

#endif