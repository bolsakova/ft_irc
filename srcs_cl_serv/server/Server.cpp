/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:01 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/06 00:23:06 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Server.hpp"
#include <sys/socket.h>   // socket, bind, listen, accept
#include <netinet/in.h>   // sockaddr_in, htons
#include <arpa/inet.h>    // inet_ntoa (если нужно)
#include <fcntl.h>        // fcntl (для non-blocking)
#include <poll.h>         // poll, struct pollfd
#include <unistd.h>       // close, read, write
#include <cstring>        // strerror, memset
#include <iostream>       // cout, cerr
#include <cerrno>         // errno



Server::Server(const char* port_str, const char* password) : m_listen_fd(-1), m_password_(password)
{

	listen_fd_ = create_and_bind(port_str);
	if (listen_fd_ < 0)
	{
		throw std::runtime_error("Failed to create and bind listening socket");
	}
	std::cout << "Server started on port " << port_str << std::endl;{
		init
	}
}
/**
 * 
Простой неблокирующий TCP-сервер с использованием poll().
Сервер принимает подключения и делает простое эхо (получил — отправил обратно).
Код предназначен как обучающий пример: аккуратно логирует события и показывает
базовую архитектуру для дальнейшего расширения (epoll, буферы, парсер IRC и т.д.).
 */

 //TODO refactor and use in initSocket
int set_non_blocking(int fd)
{
	int flags;

	// Get the current file descriptor flags
	flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1)
		return -1;
	// Set the non-blocking flag
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1)
		return -1;
	return 0;
}
// Создаёт слушающий сокет, привязывает к указанному порту и запускает listen().
// port_str — строка с номером порта (например argv[1]).
// Возвращает listen_fd (>=0) при успехе, -1 при ошибке.
// Важные шаги:
//  - socket(): создаём TCP сокет (AF_INET, SOCK_STREAM).
//  - setsockopt SO_REUSEADDR: чтобы быстро перезапускать сервер на том же порту.
//  - bind(): привязываем к INADDR_ANY и указанному порту.
//  - listen(): переводим в слушающий режим.
//  - делаем слушающий сокет неблокирующим.

int initSocket(const std::string &port_str)
{
	int port;
	
// Port 0: Reserved by the OS (means "let the system choose a port")
// Linux: valid ports are 1-65535 (TCP/IP standard, 16-bit unsigned)
// Ports 1-1023 require root/sudo privileges; use 1024+ for unprivileged apps
	// 1. Convert port to integer and check validity ===
	port = std::atoi(port_str.c_str());
	if (port <= 0 || port > 65535)
		throw std::runtime_error("Invalid port number: " + port_str);
	// 2. Create socket ===
	m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_listen_fd < 0)
	throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
	// 3.Set SO_REUSEADDR to allow quick restart after crash ===
	int opt = 1;
	if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("setsockopt() failed: " + std::string(strerror(errno)));
	}
	// 4. Bind socket to the specified 0.0.0.0:port ===
	//sockaddr_in — структура из <netinet/in.h> для описания адреса сокета IPv4
	sockaddr_in server_addr;
	// Bind socket to the specified port
	// initialize to zero
	// Способ 1: bzero (старый, BSD-стиль)
	// bzero(&server_addr, sizeof(server_addr));
	// Способ 2: memset (современный, POSIX-стиль)
	std::memset(&server_addr, 0, sizeof(server_addr)); 
	server_addr.sin_family = AF_INET; // listen on all interfaces
	server_addr.sin_addr.s_addr = INADDR_ANY; // address in network byte order(в сетевом порядке байт)
	server_addr.sin_port = htons(port); // port in network byte order(в сетевом порядке байт)

	if (bind(listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		close(listen_fd);
		throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
	}

	// 5. Start listening incoming connections non-blocking mode
	if (listen(listen_fd, SOMAXCONN) < 0)
	{
		close(listen_fd);
		throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
	}

	// 6. Set the listening socket to non-blocking mode with fcntl(), mandatory for poll()
	// Установить слушающий сокет неблокирующим — важно для неблокирующего цикла с poll.
	int flags = fcntl(m_listen_fd, F_GETFL, 0);
	if (flags < 0)
	{
		close(listen_fd);
		throw std::runtime_error("fcntl() failed: " + std::string(strerror(errno)));
	}
	if(fcntl(m_listen_fd, F_SETFL, flags | O_NONBLOCK) < 0)
	{
		close(listen_fd);
		throw std::runtime_error("fcntl() failed: " + std::string(strerror(errno)));
	}
	std::cout << "Listening on port " << port << " (non-blocking)" << std::endl;
}
