#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "../network/Client.hpp"
#include "Parser.hpp"
#include "MessageBuilder.hpp"
#include <map>
#include <memory>
#include <cctype>

// forward declaration to avoid circular dependency
class Server;

/**
 * @brief IRC command dispatcher and handler
 * 
 * Routes parsed IRC messages to appropriate handler functions
 * and generates protocol-compliant responses
 */
class CommandHandler {
	private:
			Server& m_server;					// reference to server for client access
			const std::string& m_password;		// server password for authemtication
			const std::string m_server_name;	// server name for replies

			// commands to handle
			void handlePass(Client& client, const Message& msg);	// PASS
			void handleNick(Client& client, const Message& msg);	// NICK
			void handleUser(Client& client, const Message& msg);	// USER
			void handlePing(Client& client, const Message& msg);	// PING

			bool isNicknameInUse(const std::string& nickname, int exclude_fd = -1);	// Check if nickname is already taken by another client
			bool isValidNickname(const std::string& nickname);						// Validate nickname according to RFC 1459

			void sendWelcome(Client& client);							// Send welcome messages (001-004) to newly registered client
			void sendReply(Client& client, const std::string& reply);	// Append reply to client's output buffer

	public:
			CommandHandler(Server& server, const std::string& password);	// Constructor for command handler
			~CommandHandler() = default;									// Destructor
			// deleted (contains references)
			CommandHandler(const CommandHandler&) = delete;
			CommandHandler& operator=(const CommandHandler&) = delete;

			void handleCommand(const std::string& raw_command, Client& client);	// Process a complete IRC command from client

};

#endif
