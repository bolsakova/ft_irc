#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "network/Client.hpp"
#include "Parser.hpp"
#include "MessageBuilder.hpp"
#include <map>
#include <memory>
#include <cctype>

// forward declaration to avoid circular dependency
class Server;
class Channel;

/**
 * @brief IRC command dispatcher and handler
 * 
 * Routes parsed IRC messages to appropriate handler functions
 * and generates protocol-compliant responses
 */
class CommandHandler {
	private:
			Server&	m_server;
			const	std::string& m_password;
			const	std::string m_server_name;

			// IRC command handler
			void	handlePass(Client& client, const Message& msg);
			void	handleNick(Client& client, const Message& msg);
			void	handleUser(Client& client, const Message& msg);
			void	handlePing(Client& client, const Message& msg);
			void	handleQuit(Client& client, const Message& msg);
			void	handlePrivmsg(Client& client, const Message& msg);
			void	handleJoin(Client& client, const Message& msg);
			void	handlePart(Client& client, const Message& msg);
			void	handleKick(Client& client, const Message& msg);
			void	handleInvite(Client& client, const Message& msg);
			void	handleTopic(Client& client, const Message& msg);
			void	handleMode(Client& client, const Message& msg);
			void	handleCap(Client& client, const Message& msg);
			void	handleWho(Client& client, const Message& msg);
			void	handleNotice(Client& client, const Message& msg);
			
			// MODE helpers
			void	handleUserMode(Client& client, const Message& msg, const std::string& target);
			void	handleChannelMode(Client& client, const Message& msg, const std::string& channel_name);

			// validation helpers
			bool	isNicknameInUse(const std::string& nickname, int exclude_fd = -1);
			bool	isValidNickname(const std::string& nickname);
			bool	isValidChannelName(const std::string& name);

			// response helpers
			void	sendWelcome(Client& client);
			void	sendReply(Client& client, const std::string& reply);
			void	sendError(Client& client, int error_code, const std::string& param, const std::string& message);
			void	sendNumeric(Client& client, int numeric_code, const std::string& message);
			void	broadcastToChannel(Channel& channel, const std::string& message, int exclude_fd = -1);
	public:
			CommandHandler(Server& server, const std::string& password);		// Constructor for command handler
			~CommandHandler() = default;										// Destructor
			// deleted (contains references)
			CommandHandler(const CommandHandler&) = delete;
			CommandHandler& operator=(const CommandHandler&) = delete;

			void handleCommand(const std::string& raw_command, Client& client);	// Process a complete IRC command from client

};

#endif
