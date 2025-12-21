#include "../../inc/protocol/CommandHandler.hpp"
#include "../../inc/protocol/Replies.hpp"
#include "../../inc/network/Server.hpp"

/**
 * @brief Constructor initializes the command handler with server reference
 * and server password for PASS authentication
 * 
 * @param server Reference to the main server instance
 * @param password Server password that clients mut provide
 * 
 * Algorithm:
 * 			1. Store reference to server (for accessing client list)
 * 			2. Store reference to password (for PASS validation)
 * 			3. Set server name to "ircserv" (used in all replies)
 */
CommandHandler::CommandHandler(Server& server, const std::string& password)
	: m_server(server), m_password(password), m_server_name("ircserv")
{
}

/**
 * @brief Helper function to append a reply to clientš output buffer.
 * The server's send loop will transmit it when socket is ready.
 * 
 * @param client Client to send reply to
 * @param reply Formatted IRC message (must end with \r\n)
 * 
 * Algorithm:
 * 			1. Call client.appendToOutBuf() to add reply to output queue
 * 			2. Server's poll() loop will detect POLLOUT and call sendData()
 */
void CommandHandler::sendReply(Client& client, const std::string& reply) {
	client.appendToOutBuf(reply);
}

/**
 * @brief Validate nickname according to RFC 1459 rules.
 * 
 * RFC 1459 nickname format:
 * 	- Length: 1-9 characters
 * 	- First char: letter or special = [ ] \ ` _ ^ { | }
 * 	- Other chars: letter, digit, special, or hyphen (-)
 * 
 * @param nickname Nickname string to validate
 * @return true if valid, false otherwise
 * 
 * Algorithm:
 * 			1. Check length: must be 1-9 characters
 * 			2. Check first character: must be letter or special
 * 			3. Check remaining characters: letter, digit, special, or hyphen
 * 			4. Return false if any check fails, true otherwise
 */
bool CommandHandler::isValidNickname(const std::string& nickname) {
	// Step 1: Check length constraints
	if (nickname.empty() || nickname.length() > 9)
		return false;
	
	// Step 2: Validate first character
	// Must be letter or special character from RFC 1459
	char first = nickname[0];
	if (!std::isalpha(first) &&
		first != '[' && first != ']' && first != '\\' &&
		first != '`' && first != '_' && first != '^' &&
		first != '{' && first != '|' && first != '}')
		return false;
	
	// Step 3: Validate remaining characters
	// Can be letter, digit, special, or hyphen
	for (size_t i = 1; i < nickname.length(); ++i) {
		char c = nickname[i];
		if (!std::isalnum(c) &&
			c != '[' && c != ']' && c != '\\' && 
        	c != '`' && c != '_' && c != '^' && 
        	c != '{' && c != '|' && c != '}')
			return false;
	}

	return true;
}

/**
 * @brief Check if a nickname is already in use by another client
 * Used to prevent duplicate nicknames on the server
 * 
 * @param nickname Nickname to check
 * @param exclude_fd File descriptor to exclude (for nick changes)
 * @return true if nickname is taken, false if available
 * 
 * Algorithm:
 * 			1. Get reference to all connected clients from server
 * 			2. Iterate through each client in the map
 * 			3. Skip the client with exclude_fd (allows client to keep same nick)
 * 			4. Compare nicknames (case-sensitive in IRC)
 * 			5. Return true if match found, false if nickname is free
 */
bool CommandHandler::isNicknameInUse(const std::string& nickname, int exclude_fd) {
	// Get reference to server's client map
	const std::map<int, std::unique_ptr<Client>>& clients = m_server.getClients();

	// Iterate through all connected clients
	for (std::map<int, std::unique_ptr<Client>>::const_iterator it = clients.begin();
		it != clients.end(); ++it)
	{
		// Skip the client we want to exclude (typically the one changing nick)
		if (it->first == exclude_fd)
			continue;

		// Check if this client has the nickname
		if (it->second->getNickname() == nickname)
			return true;
	}

	return false;
}
