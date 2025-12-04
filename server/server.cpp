/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:01 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/04 00:36:30 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <sys/socket.h>   // socket, bind, listen, accept
#include <netinet/in.h>   // sockaddr_in, htons
#include <arpa/inet.h>    // inet_ntoa (если нужно)
#include <fcntl.h>        // fcntl (для non-blocking)
#include <poll.h>         // poll, struct pollfd
#include <unistd.h>       // close, read, write
#include <cstring>        // strerror, memset
#include <iostream>       // cout, cerr
#include <cerrno>         // errno

/**
 * 
Простой неблокирующий TCP-сервер с использованием poll().
Сервер принимает подключения и делает простое эхо (получил — отправил обратно).
Код предназначен как обучающий пример: аккуратно логирует события и показывает
базовую архитектуру для дальнейшего расширения (epoll, буферы, парсер IRC и т.д.).
 */
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
//  - set_nonblocking(): делаем слушающий сокет неблокирующим.

int create_and_bind(const char *port_str)
{
	int listen_fd;
	int port;
	// struct sockaddr_in server_addr;
	
// Port 0: Reserved by the OS (means "let the system choose a port")
// Linux: valid ports are 1-65535 (TCP/IP standard, 16-bit unsigned)
// Ports 1-1023 require root/sudo privileges; use 1024+ for unprivileged apps
	port = std::atoi(port_str);
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Invalid port number: " << port_str << std::endl;
		return -1;
	}

	// Разрешаем быстрое повторное привязывание к этому порту (убирает TIME_WAIT/ADDRESS_IN_USE в dev).
	// Create socket
	int opt = 1;
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0)
	{
		perror("socket");
		return -1;
	}

	// Set SO_REUSEADDR to allow quick restart
	if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		perror("setsockopt");
		close(listen_fd);
		return -1;
	}
	struct sockaddr_in server_addr;
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
		perror("bind");
		close(listen_fd);
		return -1;
	}

	// Start listening incoming connections non-blocking mode
	if (listen(listen_fd, SOMAXCONN) < 0)
	{
		perror("listen");
		close(listen_fd);
		return -1;
	}

	// Set the listening socket to non-blocking mode
	// Установить слушающий сокет неблокирующим — важно для неблокирующего цикла с poll.
	if (set_non_blocking(listen_fd) < 0)
	{
		perror("set_non_blocking");
		close(listen_fd);
		return -1;
	}

	return listen_fd;
}
