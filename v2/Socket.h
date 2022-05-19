#ifndef __SOCKET_H__
#define __SOCKET_H__
/**
 * @file Socket.h
 * @author vilewind 
 * @brief 封装socket，与服务器端创建listenfd和accept连接相关
 * @version 0.1
 * @date 2022-05-19
 * 
 * @copyright Copyright (c) 2022
 * 
 */
#include "general_header.h"

#include <string>
#include <memory>

class Socket 
{
public:
	Socket(const std::string &host, const int port):m_addr(new sockaddr_in) {
		m_addr->sin_family = AF_INET;
		m_addr->sin_port = htons(port);
		int ret = funcException("inet_pton a host", inet_pton, AF_INET, host.c_str(), &m_addr->sin_addr);
		if (ret == 0) {
			std::cerr << "host does not contain a character string representing a valid network address in the specified address family." << std::endl;
			exit(-1);
		}
		
		m_fd = funcException("socket create", socket, PF_INET, SOCK_STREAM, 0);

		addrReuse(m_fd);

		Bind();

		Listen();
	}
	~Socket() {
		close(m_fd);
	}
public:
	int getListenfd() const { return m_fd; }
	
	int Accept() {
		sockaddr_in cli_addr;
		socklen_t len = sizeof(cli_addr);
		int cli_fd = funcException("accept a new client", accept, m_fd, (sockaddr*)&cli_addr, &len);
		
		return cli_fd;
	}

private:
/**
 * @attention 由于bind具有多个默认定义，在作为函数参数传入时，需要使用作用域符号"::"，加以区分
 * 
 */
	void Bind() {
		int ret = funcException("bind a listenfd", ::bind, m_fd, (sockaddr *)(m_addr.get()), sizeof(sockaddr_in));
	}

	void test(int a) {
		std::cout << a << std::endl;
	}
	void Listen(){
		int ret = funcException("listening", listen, m_fd, 10);
	}

private:
	int m_fd;
	std::shared_ptr<sockaddr_in> m_addr;
	// sockaddr_in *m_addr;
};

#endif