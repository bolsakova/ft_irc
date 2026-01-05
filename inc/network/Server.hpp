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
 * Server is designed as a single, non-copyable, non-movable object:
 * Default constructor — forbidden (we have a custom constructor)
 * Copy constructor and assignment operator — forbidden
 */
class Server
{
	private:
			int		m_listen_fd;
			bool	m_running;
			std::string	m_password;
			std::vector<pollfd>	m_poll_fds;								// all descriptors tracked by poll()
			std::map<int, std::unique_ptr<Client>>	m_clients;			// fd→Client; one owner, auto cleanup (whithout delete), no leaks, exception-safe - if cnst/function throws, memory freed automatically
			std::map<std::string, std::unique_ptr<Channel>>	m_channels;	// name→Channel; server owns, auto-cleanup on erase/destruction
			std::unique_ptr<CommandHandler>	m_cmd_handler;

			void	initSocket(const std::string &port);
			void	acceptClient();
			bool	receiveData(int fd);
			void	sendData(int fd);
			void	disconnectClient(int fd);
			void	cleanupDisconnectedClients();
	
	public:
			// Deleted OCF methods (canonical but disabled)
			Server() = delete;
			Server(const Server& other) = delete;
			Server&	operator=(const Server& other) = delete;
			
			Server(const std::string &port, const std::string &password);
			~Server();
			void		run();
			void		stop();
			const std::map<int, std::unique_ptr<Client>>& getClients() const;
			Channel*	findChannel(const std::string& name);
			Client*		findClientByNickname(const std::string& nickname);
			void		addClient(int fd, std::unique_ptr<Client> client);
			Channel*	createChannel(const std::string& name);
			void		removeChannel(const std::string& name);
			const std::map<std::string, std::unique_ptr<Channel>>& getChannels() const;
			void		enablePolloutForFD(int fd);
			void		disablePolloutForFd(int fd);
};

#endif
