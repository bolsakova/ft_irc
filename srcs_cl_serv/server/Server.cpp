/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:01 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/14 23:03:34 by aokhapki         ###   ########.fr       */
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
#include <cerrno> 

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
Server::Server(const std::string& port, const std::string& password)
	: m_listen_fd(-1), m_password(password)
{

	initSocket(port);
	if (m_listen_fd < 0)
	{
		std::cerr << "Failed to initialise socket on port " << port << "\n";
		return;
	}
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

	// 2. Close all client sockets (fd are system resources)
	for (auto &pair : m_clients)
	{
		int fd = pair.first;
		if (fd >= 0)
			close(fd);
	}

	// 3. Explicitly clear containers (optional but makes intent clear)
	// unique_ptr will automatically delete Client objects
	m_clients.clear();
	m_poll_fds.clear();

	std::cout << "Server destroyed: all sockets closed, all clients removed." << std::endl;

	// Notes:
	// - Calling clear() is optional:
	//   m_clients will be destroyed automatically when Server is destroyed.
	// - std::vector<m_poll_fds> also frees its memory automatically in its destructor.
	// - clear() is used here only for clarity and explicit resource release order.
	// используется здесь только для ясности и четкого порядка освобождения ресурсов
	
}

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

void Server::initSocket(const std::string &port_str)
{
	int port;
	
// Port 0: Reserved by the OS (means "let the system choose a port")
// Linux: valid ports are 1-65535 (TCP/IP standard, 16-bit unsigned)
// Ports 1-1023 require root/sudo privileges; use 1024+ for unprivileged apps
// 1. Convert port to integer and check validity ===
	port = std::atoi(port_str.c_str()); // TODO уточнить можно ли использовать stoi() port = std::stoi(port_str);
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
	std::cout << "Listening on port " << port << " (non-blocking)" << std::endl;
	return;
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
		socklen_t   addr_len = sizeof(client_addr);

// accept() забирает одно входящее подключение из очереди
// accept хочет sockaddr*, у нас sockaddr_in, поэтому явно приводим 
// &client_addr к sockaddr*, чтобы передать IPv4‑структуру туда, где ожидается базовый адрес.
		int client_fd = accept(m_listen_fd,
							   reinterpret_cast<sockaddr*>(&client_addr),
							   &addr_len);
		if (client_fd < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			// Любая другая ошибка — просто логируем и выходим из acceptClient
			std::cerr << "accept() failed: " << std::strerror(errno) << std::endl;
			break;
		}
		// Делаем клиентский сокет неблокирующим
		if (set_non_blocking(client_fd) < 0)
		{
			std::cerr << "set_non_blocking() failed for client fd "
					  << client_fd << ": " << std::strerror(errno) << std::endl;
			close(client_fd);
			continue; // пробуем принять следующего клиента
		}
		// Добавляем клиентский сокет в poll()
		pollfd client_pfd;
		client_pfd.fd      = client_fd;
		client_pfd.events  = POLLIN; // хотим читать данные от клиента
		client_pfd.revents = 0;
		m_poll_fds.push_back(client_pfd);
		// Создаем объект Client и сохраняем указатель в m_clients
		// Client *client = new Client(client_fd);
		// m_clients[client_fd] = client;
		// m_clients[client_fd] = std::make_unique<Client>(client_fd); //with copy constructor
		m_clients.emplace(client_fd, std::make_unique<Client>(client_fd)); //without copy constructor
		// Лог/отладка
		std::cout << "New client accepted, fd = " << client_fd << std::endl;
	}
}
void Server::disconnectClient(int fd)
{
	// 1. Close socket
	if (fd >= 0)
		close(fd);

	// 2. Remove from clients map (unique_ptr frees memory)
	m_clients.erase(fd);

	// 3. Remove fd from poll fds
	for (size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		if (m_poll_fds[i].fd == fd)
		{
			m_poll_fds.erase(m_poll_fds.begin() + i);
			break;
		}
	}

	std::cout << "Client fd " << fd << " disconnected and removed." << std::endl;
}

void Server::enablePolloutForFD(int fd)
{
	// 1) Проходим по всем отслеживаемым дескрипторам poll()
	for(size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		// 2) Ищем нужный fd
		if(m_poll_fds[i].fd == fd)
		{
			// 3) Добавляем флаг POLLOUT, чтобы poll() уведомил о готовности писать
			m_poll_fds[i].events = m_poll_fds[i].events | POLLOUT;
			// 4) Выходим, как только настроили нужный дескриптор
			return
		}
	} 
}

void Server::receiveData(int fd)
{
	char buffer[4096];
	ssize_t bytes_read;
// recv() читает данные из сокета.
// Для неблокирующего сокета:
//  - >0  → прочитали столько-то байт
//  -  0  → клиент закрыл соединение
//  - <0  → ошибка (в т.ч. EAGAIN/EWOULDBLOCK)
	bytes_read = recv(fd, buffer, sizeof(buffer), 0);
	if (bytes_read < 0)
	{
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		// любая другая ошибка — логируем и отключаем клиента
		std::cerr << "recv() failed on fd " << fd << ": "
				  << std::strerror(errno) << std::endl;
		disconnectClient(fd);
		return;
	}
	if (bytes_read == 0)
	{
		// 0 байт → клиент аккуратно закрыл соединение (EOF)
		std::cout << "Client fd " << fd << " disconnected (EOF)" << std::endl;
		disconnectClient(fd); 
		return;
	}
	// Преобразую принятые байты в std::string
	std::string data(buffer, bytes_read);
	// Находим клиента по fd
	// std::map<int, Client *>::iterator it = m_clients.find(fd);
	auto it = m_clients.find(fd);
	if (it == m_clients.end())
	{
		// Теоретически не должно случаться, но на всякий случай проверяем
		std::cerr << "receiveData(): no Client object for fd " << fd << std::endl;
		return;
	}
	Client &client = *(it->second);// получаем ссылку на Client объект
	// 1. Добавляем новые данные в входной буфер клиента.
	//    Здесь мы не предполагаем, что получили ровно одну команду.
	client.appendToInBuf(data);
	// 2. Достаём из буфера все полные команды, которые есть.
	//    IRC-команды заканчиваются на "\r\n".
	while (client.hasCompleteCmd())
	{
		std::string cmd = client.extractNextCmd();
		// На этом этапе у нас есть одна "цельная" строка-команда.
		// Пока Таня пишет протокольную часть, делаю простой echo для проверки.
		std::cout << "Received command from fd " << fd << ": [" << cmd << "]\n";
		// Формируем ответ (пока echo, потом будет protocol): отправим обратно ту же команду с префиксом и \r\n
		std::string response = "ECHO: " + cmd + "\r\n";
		// send() может отправить не все байты, поэтому используем выходной буфер, который добавляет 
		// строки к существующему стрингу
		client.appendToOutBuf(response);
		// 3) Включаем POLLOUT для этого fd,
		// чтобы poll() разбудил нас, когда сокет готов писать.
		enablePolloutForFD();
	}
}
void Server::sendData(int fd)
{
	//  Берём ссылку на клиента по его дескриптору, копии запрещены, работаем только с существующим обьектом
	Client &client = *m_clients[fd];
	
	//  Если в исходящем буфере ничего нет — сразу выходим
	if(client.m_outbuf.empty())
		return;
	//  Пытаемся отправить содержимое буфера в сокет
	ssize_t sent = send(fd,
			client.m_outbuf.c_str,
			client.m_outbuf.size, 
			0);
	//  Обработка ошибок send()
	if(sent < 0)
	{
		if(errno == EAGAIN || errno == EWOULDBLOCK)
		// Сокет временно не готов к записи — попробуем позже
			return;
		// Любая другая ошибка — отключаем клиента
		disconnectClient(fd);
		return;
	}
	//  Удаляем из буфера уже отправленную часть
	client.m_outbuf.erase(0, sent);
}

void Server::run()
{
	// 1. Добавляем слушающий сокет в вектор pollfd.
	// Этот вектор говорит poll(), какие дескрипторы мы хотим отслеживать.
	m_poll_fds.clear();
	pollfd listen_pfd;
	listen_pfd.fd = m_listen_fd; 
	listen_pfd.events = POLLIN; // always= POLLIN, we say: listen incomming -> poll() blocked and waits for incoming events
	listen_pfd.revents = 0; //when Client makes something (connects or write a msg) poll() awakes and fills revents and returns number of fds with events
	m_poll_fds.push_back(listen_pfd); //push_back копирует структуру pollfd и добавляет в вектор
	std::cout << "Server is running and waiting for connections..." << std::endl; 

	// 2. Основной цикл сервера с poll() работает, пока сервер не завершат извне
	while(true)
	{
		// poll(массив pollfd, кол-во элементов, таймаут -1=ждать∞)
		// возвращает: кол-во готовых fd, 0 - таймаут, -1 - ошибка
		int ret = poll(m_poll_fds.data(), m_poll_fds.size(), -1); // static_cast<nfds_t>(m_poll_fds.size()) не требуется.  g++ -Wsign-conversion  # может выдать warning, но это при строгих настройках, оч редко
		if (ret < 0)
		{
			// EINTR = сигнал прервал poll(), это не опасная ошибка (просто сигнал), продолжаем цикл
			if (errno == EINTR)
				continue;
			// Любая другая ошибка — считаем фатальной
			throw std::runtime_error("poll() failed: " + std::string(strerror(errno)));
		}
		if (m_poll_fds.empty())
			continue;
		// 3. Проходим по всем отслеживаемым дескрипторам и смотрим, у кого есть события
		// ВАЖНО: мы читаем fd и revents в локальные переменные, потому что
		// внутри цикла можем вызывать disconnectClient(), который изменит m_poll_fds.
		// for (size_t i = 0; i < m_poll_fds.size(); ++i) переписала, чтобы начинать обход с конца вектора для корректного удаления элементов внутри цикла
		for (int i = static_cast<int>(m_poll_fds.size()) - 1; i >= 0; --i)// int i вместо size_t, чтобы избежать предупреждений компилятора при сравнении с ssize_t

		{
			int fd = m_poll_fds[i].fd;
			short revents = m_poll_fds[i].revents;
			// Если нет событий, переходим к следующему дескриптору
			if (revents == 0)
				continue;
				// 4. Обрабатываем события для каждого дескриптора
				// Событие на слушающем сокете — значит, есть новые входящие подключения
			if (fd == m_listen_fd)
			{
				// Слушающий сокет - Принимаем одного или несколько клиентов
				acceptClient();
			}
			else
			{
				// 1. Сначала проверяем ошибки/разрыв соединения
				// Клиентский сокет — данные от клиента или отключение
				if (revents & (POLLHUP | POLLERR))
				{
					// Если произошла ошибка или клиент повесил трубку — отключаем его
					disconnectClient(fd);
					continue;  // важно: не идём в POLLIN для этого fd
				}
				// 2. Только если нет ошибок — читаем данные
				if (revents & POLLIN)
					receiveData(fd);
				// 3. Потом пишем 
				if (revents & POLLOUT)
					sendData(fd);
			}	
			// Почему порядок read → write норм: часто после чтения ты добавляешь данные в outbuf
			// и тут же в этом же цикле можно попробовать отправить
		}
	}
}
