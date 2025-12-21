#ifndef COMMANDHANDLER_HPP
#define COMMANDHANDLER_HPP

#include "../network/Client.hpp"
#include "Parser.hpp"
#include "Message.hpp"

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
			Server& server;					// reference to server for client access
			const std::string& password;	// server password for authemtication
			const std::string server_name;	// server name for replies

			/**
			 * @brief Handle PASS command (password authentication)
			 * 
			 * @param client Client sending the command
			 * @param msg Parsed message structure
			 */
			void handlePass(Client& client, const Message& msg);
			
			/**
			 * @brief Handle NICK command (set/change nickname)
			 * 
			 * @param client Client sending the command
			 * @param msg Parsed message structure
			 */
			void handleNick(Client& client, const Message& msg);
			
			/**
			 * @brief Handle USER command (set username and realname)
			 * 
			 * @param client Client sending the command
			 * @param msg Parsed message structure
			 */
			void handleUser(Client& client, const Message& msg);

			/**
			 * @brief Check if nickname is already taken by another client
			 * 
			 * @param nickname Nickname to check
			 * @param exclude_fd File descriptor to exclude from check (for nick changes)
			 * @return True if nickname is is use, false otherwise
			 */
			bool isNicknameInUse(const std::string& nickname, int exclude_fd = -1);

			/**
			 * @brief Validate nickname according to RFC 1459
			 * 
			 * @param nickname Nickname to validate
			 * @return True if valid, false otherwise
			 */
			bool isValidNickname(const std::string& nickname);

			/**
			 * @brief Send welcome messages (001-004) to newly registered client
			 * 
			 * @param client Client to send welcome to
			 */
			void sendWelcome(Client& client);

			/**
			 * @brief Append reply to client's output buffer
			 * 
			 * @param client Client to send to
			 * @param reply Formatted IRC reply string
			 */
			void sendReply(Client& client, const std::string& reply);

	public:

};

#endif
