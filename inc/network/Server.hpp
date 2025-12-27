/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:10 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/16 19:33:59 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <sys/poll.h>
#include "Client.hpp"
#include "Channel.hpp"

class CommandHandler;

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
	std::vector<pollfd> m_poll_fds; // все дескрипторы, которые отслеживает poll()
	std::map<int, std::unique_ptr<Client>> m_clients; // переход на std::unique_ptr<Client>  к безопасному и современному C++17.
	std::map<std::string, std::unique_ptr<Channel>> m_channels; // Каналы по имени, владеем их объектами
	std::unique_ptr<CommandHandler> m_cmd_handler;

	void initSocket(const std::string &port);
	// обработка событий
	void acceptClient();
	void receiveData(int fd);
	void sendData(int fd);
	void enablePolloutForFD(int fd);
	void disablePolloutForFd(int fd);
	void disconnectClient(int fd);
	
	public:
	// Deleted OCF methods (canonical but disabled)
	Server() = delete;
	Server(const Server& other) = delete;
	Server& operator=(const Server& other) = delete;
	
	Server(const std::string &port, const std::string &password);
	~Server();
	void run();
	const std::map<int, std::unique_ptr<Client>>& getClients() const;
	Channel* findChannel(const std::string& name);
	// TANJA
	Client* findClientByNickname(const std::string& nickname);
	void addClient(int fd, std::unique_ptr<Client> client);

	Channel* createChannel(const std::string& name);
	void removeChannel(const std::string& name);
	const std::map<std::string, std::unique_ptr<Channel>>& getChannels() const;
};

#endif
