/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:10 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/06 09:00:47 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP


#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <sys/poll.h>
#include "Client.hpp"

/**
 * Класс Server — объект, который должен быть: только один, не копируемый, не перемещаемый. Значит:
 * Default constructor — forbidden (we have custom constructor)
 * Copy constructor and assignment operator — forbidden
 */

class Server
{
	private:
	int m_listen_fd;
	std::string m_password;
	std::vector<pollfd> m_poll_fds;
	std::map<int, Client> m_clients;

	// инициация сокета
	void initSocket(const std::string &port);

	// обработка событий
	void acceptСlient();
	void receivData(int fd);
	void disconnectClient(int fd);

	public:
	// Deleted OCF methods (canonical but disabled)
    Server() = delete;
    Server(const Server& other) = delete;
    Server& operator=(const Server& other) = delete;
	
	Server(const std::string &port, const std::string &password);
	~Server();
	void run();
};

#endif