/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:01 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/16 23:46:35 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <cstring>        // strerror, memset
#include <iostream>       // cout, cerr
#include <cerrno>         // errno
#include <csignal>        // signal/sigaction (SIGPIPE)
#include <fcntl.h>        // fcntl (для non-blocking)
#include <unistd.h>       // close, read, write
#include <sys/socket.h>   // socket, bind, listen, accept
#include <netinet/in.h>   // sockaddr_in, htons
#include <arpa/inet.h>    // inet_ntoa (если нужно)
#include "../../inc/network/Server.hpp"
#include "../../inc/network/net.hpp"
#include "../../inc/protocol/CommandHandler.hpp"

/*EAGAIN/EWOULDBLOCK - больше нет ожидающих подключений
non-blocking and interrupt: обрабатывают и пробуют снова позже, не падая.-> временно нет данных, пробуем позже, не падая.
================
events waiting for:
POLLIN — данные готовы к чтению
POLLOUT — сокет готов к записи
revents returnes:
POLLIN — данные готовы к чтению
POLLOUT — сокет готов к записи
POLLERR — произошла ошибка
POLLHUP — разрыв соединения
*/

static void ignore_sigpipe()
{
	// CHANGED: Protect the server from being killed by SIGPIPE when sending to a closed socket.
	// This is critical for network servers using send().
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);
}

Server::Server(const std::string& port, const std::string& password)
	: m_listen_fd(-1), m_password(password)
{
	ignore_sigpipe();
	initSocket(port);
	if (m_listen_fd < 0)
	{
		std::cerr << "Failed to initialise socket on port " << port << "\n";
		return;
	}
	// Создаём CommandHandler после успешной инициализации сокета
	m_cmd_handler = std::make_unique<CommandHandler>(*this, m_password);
	std::cout << "Server started on port " << port << std::endl;
}

// unique_ptr automatically frees memory.
// We do NOT need to delete Client manually.
// Client objects are destroyed when:
//  - an element is erased from m_clients
//  - m_clients.clear() is called
//  - or when m_clients itself is destroyed as a class member
Server::~Server()
{
	// 1. Close the listening socket
	if (m_listen_fd >= 0)
		close(m_listen_fd);
	// 2. Close all client sockets
	// unique_ptr will auto-delete Client objects
	for (std::map<int, std::unique_ptr<Client>>::iterator it = m_clients.begin();
		 it != m_clients.end(); ++it)
	{
		if (it->first >= 0)
			close(it->first);
	}
	// 3. Explicitly clear containers (optional but makes intent clear)
	// unique_ptr will automatically delete Client objects
	m_clients.clear();
	m_poll_fds.clear();
}

// Найти канал по имени (возвращает nullptr, если не найден).
Channel* Server::findChannel(const std::string& name)
{
	std::map<std::string, std::unique_ptr<Channel> >::iterator it = m_channels.find(name);
	if (it == m_channels.end())
		return NULL;
	return it->second.get();
}

// TANJA: Найти клиента по nickname
Client* Server::findClientByNickname(const std::string& nickname)
{
    for (std::map<int, std::unique_ptr<Client>>::iterator it = m_clients.begin();
        it != m_clients.end(); ++it)
    {
        if (it->second->getNickname() == nickname)
            return it->second.get();
    }
    return NULL;
}

// TANJA: Добавить клиента в карту клиентов (для тестов)
void Server::addClient(int fd, std::unique_ptr<Client> client)
{
    m_clients[fd] = std::move(client);
}

// Создать канал, если его ещё нет; вернуть указатель на существующий/новый.
Channel* Server::createChannel(const std::string& name)
{
	Channel* existing = findChannel(name);
	if (existing)
		return existing;
	std::unique_ptr<Channel> ch(new Channel());
	Channel* raw = ch.get();
	m_channels[name] = std::move(ch);
	return raw;
}

// Удалить канал по имени (если существует).
void Server::removeChannel(const std::string& name){m_channels.erase(name);}

// Получить карту всех каналов (read-only доступ).
const std::map<std::string, std::unique_ptr<Channel>>& Server::getChannels() const{return m_channels;}

void Server::initSocket(const std::string &port_str)
{
	int port;
// Port 0: Reserved by the OS (means "let the system choose a port")
// Linux: valid ports are 1-65535 (TCP/IP standard, 16-bit unsigned)
// Ports 1-1023 require root/sudo privileges; use 1024+ for unprivileged apps
// 1. Convert port to integer and check validity ===
	port = parse_port_strict(port_str); // CHANGED: strict validation (no partial parse)
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

	if (bind(m_listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
	}
	// 5. Start listening incoming connections non-blocking mode
	if (listen(m_listen_fd, SOMAXCONN) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
	}
	// 6. Make socket NON-BLOCKING, mandatory for poll() 
	if (set_non_blocking(m_listen_fd) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("set_non_blocking() failed: " + std::string(strerror(errno)));
	}
	// std::cout << "Listening on port " << port << " (non-blocking)" << std::endl;
	// return;
}

/*
IPv4 xxx.xxx.xxx.xxx 32-бит→ всего ≈ 4.3 миллиарда адресов.
IPv6 xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx 128-бит → ≈ 340 секстиллионов адресов (> чем атомов в Солнечной системе).
socklen_t - POSIX определяет универсальный и переносимый тип для хранения длины адресных структур в сетевых вызовах.
работает на любых ОС (32 и 64 битных), где типы могут отличаться размерами.
*/
void Server::acceptClient()
{
	while (true)
	{
		sockaddr_in client_addr;
		socklen_t client_len;
		
		client_len = sizeof(client_addr);
		int client_fd = accept(m_listen_fd, (sockaddr *)&client_addr, &client_len);
		if (client_fd < 0)
		{
			// accept() on non-blocking socket: при отсутствии ожидающих подключений accept возвращает -1 - EAGAIN/EWOULDBLOCK means: no more pending connections (normal)
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			// any other error — log it
			std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
			break;
		}
		if (set_non_blocking(client_fd) < 0)// Set client socket non-blocking
		{
			std::cerr << "Failed to set client non-blocking, fd " << client_fd << "\n";
			close(client_fd);
			continue;
		}
		// Add to poll list
		pollfd pfd;
		pfd.fd = client_fd;
		pfd.events = POLLIN;  // start with only read events
		pfd.revents = 0;
		m_poll_fds.push_back(pfd);//push_back копирует структуру pollfd и добавляет в вектор
		m_clients.emplace(client_fd, std::make_unique<Client>(client_fd)); //without copy constructor
		std::cout << "New client accepted, fd = " << client_fd << std::endl;
	}
}

void Server::disconnectClient(int fd)
{
    // 1. Remove fd from poll fds
    for (size_t i = 0; i < m_poll_fds.size(); ++i)
    {
        if (m_poll_fds[i].fd == fd)
        {
            m_poll_fds.erase(m_poll_fds.begin() + i);
            break;
        }
    }
    // 2. Close socket (free OS resource)
    if (fd >= 0)
        close(fd);
    // 3. Remove from clients map (unique_ptr frees memory)
    m_clients.erase(fd);
	std::cout << "Client fd " << fd << " disconnected and removed." << std::endl;
}

void Server::enablePolloutForFD(int fd)
{
	// 1. Проходим по всем отслеживаемым дескрипторам
	for(size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		// 2. Ищем нужный fd
		if(m_poll_fds[i].fd == fd)
		{
			// 3. Добавляем флаг POLLOUT, чтобы poll() ждал готовности к записи
			m_poll_fds[i].events = m_poll_fds[i].events | POLLOUT;
			// 4). Выходим после обновления нужного дескриптора
			return;
		}
	}
}

void Server::disablePolloutForFd(int fd)
{
	// 1. Проходим по всем отслеживаемым дескрипторам
	for(size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		// 2. Ищем нужный fd
		if(m_poll_fds[i].fd == fd)
		{
			// 3. Убираем флаг POLLOUT, чтобы poll() больше не ждал готовности к записи
			m_poll_fds[i].events = m_poll_fds[i].events & (~POLLOUT);
			// 4. Выходим после обновления нужного дескриптора
			return;
		}
	}
}


/**
recv() reads data from the socket.
For a non-blocking socket:
- >0  → read this nb of bytes
- =0  → client closed the connection
- <0  → error (including EAGAIN/EWOULDBLOCK)
*/
void Server::receiveData(int fd)
{
	// read in a loop until EAGAIN/EWOULDBLOCK (better for performance and correctness)
	// because poll() is level-triggered.
	char buffer[4096];
	ssize_t bytes_read;
	while (true)
	{
		bytes_read = recv(fd, buffer, sizeof(buffer), 0);
		if (bytes_read < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break; // no more data for now
			// Any other error — log and disconnect
			std::cerr << "recv() failed on fd " << fd << ": "
					  << std::strerror(errno) << std::endl;
			disconnectClient(fd);
			return;
		}
		if (bytes_read == 0)
		{
			// peer closed input (EOF). We must NOT drop connection immediately
			// if we still have data in outbuf to send (important for: printf | nc tests).
			std::cout << "Client fd " << fd << " closed input (EOF).\n";
			auto it = m_clients.find(fd);
			if (it == m_clients.end())
				return;
			Client &client = *(it->second);
			client.markPeerClosed();
			// Stop reading: no more POLLIN. But keep POLLOUT to flush outbuf.
			disable_pollevent(m_poll_fds, fd, POLLIN);
			// If nothing to send - can close now
			if (!client.hasDataToSend())
				disconnectClient(fd);
			return;
		}
		// Find client safely (do NOT use operator[] here).
		auto it = m_clients.find(fd);
		if (it == m_clients.end())
			return; // client already removed
		Client &client = *(it->second);
		// Append received chunk to the input buffer
		std::string data(buffer, static_cast<std::size_t>(bytes_read));
		// proper overflow check with incoming chunk size
		const std::size_t MAX_INBUF = 8192;
		if (client.getInBuf().size() + data.size() > MAX_INBUF)
		{
			std::cerr << "Input buffer overflow for fd " << fd
					  << " (limit " << MAX_INBUF << ")\n"; // keep \n escaped
			disconnectClient(fd);
			return;
		}
		client.appendToInBuf(data);
		// Process all complete commands currently in buffer
		while (client.hasCompleteCmd())
		{
			std::string cmd = client.extractNextCmd();
			// std::cout << "Received command from fd " << fd << ": [" << cmd << "]\n"; //можно удалить если не нужно в аутпут
			// Передаём команду в CommandHandler для обработки
			m_cmd_handler->handleCommand(cmd, client);
			// Если есть что отправить - включаем POLLOUT
			if (client.hasDataToSend())
				enablePolloutForFD(fd);
		}
	}
}

const std::map<int, std::unique_ptr<Client>>& Server::getClients() const
{
	return m_clients;
}

void Server::sendData(int fd)
{
	// never use m_clients[fd] (operator[] can create a new empty entry!)
	auto it = m_clients.find(fd);
	if (it == m_clients.end())
		return;
	Client &client = *(it->second);
	//  Если в исходящем буфере ничего нет — сразу выходим
	if (!client.hasDataToSend())
	{
		disablePolloutForFd(fd);
		return;
	}
	//  Пытаемся отправить содержимое буфера в сокет
	const std::string &out = client.getOutBuf();
	// protect from SIGPIPE on Linux with MSG_NOSIGNAL (extra safety).
	// We still ignore SIGPIPE globally, but this makes send() itself safer.
	int flags = 0;
	#ifdef MSG_NOSIGNAL
		flags = MSG_NOSIGNAL;
	#endif
	ssize_t sent = send(fd, out.c_str(), out.size(), flags);
	//  Обработка ошибок send()
	if (sent < 0)
	{	// Сокет временно не готов к записи — попробуем позже
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		// Любая другая ошибка — отключаем клиента
		std::cerr << "send() failed on fd " << fd << ": "
				  << std::strerror(errno) << std::endl; // CHANGED: log errno
		disconnectClient(fd);
		return;
	}
	//  Удаляем из буфера уже отправленную часть
	client.consumeOutBuf(static_cast<std::size_t>(sent));
	// If buffer is empty — stop watching POLLOUT
	if (!client.hasDataToSend())
		disablePolloutForFd(fd);
	// If peer already closed and we finished sending,now we can close our side too
	// if (client.isPeerClosed()) // TODO add && !client.hasDataToSend()), delete after testing
	// disconnect only after we flushed everything
	if (client.isPeerClosed() && !client.hasDataToSend())
		disconnectClient(fd);
	return;
}

void Server::run()
{
	// 1. Подготовка массива pollfd, добавляем слушающий сокет
	pollfd listen_poll;
	listen_poll.fd = m_listen_fd;
	listen_poll.events = POLLIN;
	listen_poll.revents = 0;
	m_poll_fds.push_back(listen_poll);
	//push_back копирует структуру pollfd и добавляет в вектор
	std::cout << "Server is running and waiting for connections..." << std::endl; 
	// 2. Основной цикл сервера с poll() работает, пока сервер не завершат извне
	while(true)
	{
		// poll() блокируется до появления событий или таймаута
		// -1 timeout means wait forever
		int ready = poll(&m_poll_fds[0], m_poll_fds.size(), -1);
		if (ready < 0)
		{
			// poll interrupted by signal, can continue
			if (errno == EINTR)
				continue;
			std::cerr << "poll() failed: " << std::strerror(errno) << std::endl; // CHANGED
			break;
		}
		// 3. Обходим все fd и обрабатываем события
		// Важно: если мы будем удалять элементы из m_poll_fds во время итерации,
		// нужно аккуратно обновлять индексы. Самый простой способ — проходить с конца.
		for (int i = static_cast<int>(m_poll_fds.size()) - 1; i >= 0; --i)
		{
			int fd = m_poll_fds[i].fd;
			short revents = m_poll_fds[i].revents;
			// если нет событий — пропускаем
			if (revents == 0)
				continue;
			// сбрасываем revents, чтобы не обрабатывать повторно
			m_poll_fds[i].revents = 0;
			if (fd == m_listen_fd)
				acceptClient();// Слушающий сокет - Принимаем одного или несколько клиентов
			else
			{
				// 1. Сначала проверяем ошибки/разрыв соединения
				// POLLERR => disconnect immediately
				if (revents & POLLERR)
				{
					disconnectClient(fd);
					continue;  // важно: не идём в POLLIN для этого fd
				}
				// 2. Read first (VERY IMPORTANT: even if POLLHUP (hung up повесил трубку) is also set)
				if (revents & POLLIN)
					receiveData(fd);
				// 3. receiveData() мог отключить клиента
				if (m_clients.find(fd) == m_clients.end())
					continue;
				// 4. Then write
				if (revents & POLLOUT)
					sendData(fd);
			}	
			// Почему порядок read → write норм: часто после чтения добавлям данные в outbuf
			// recv()==0 inside receiveData is the correct signal for disconnect.
		}
	}
}
