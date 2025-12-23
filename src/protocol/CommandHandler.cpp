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

/**
 * @brief Send welcome messages (RPL_WELCOME through RPL_MYINFO) to client.
 * Called after successful registration (PASS + NICK + USER complete)
 * 
 * @param client Newly registered client
 * 
 * Algorithm:
 * 			1. Build RPL_WELCOME (001): "Welcome to the IRC Network nick!user@host"
 * 			2. Build RPL_YOURHOST (002): "Your host is servername, version"
 * 			3. Build RPL_CREATED (003): "This server was created <date>"
 * 			4. Build RPL_MYINFO (004): "servername version user_modes chan_modes"
 * 			5. Send each reply to client's output buffer
 */
void CommandHandler::sendWelcome(Client& client) {
	// RPL_WELCOME (001): Welcome message with full client identifier
	std::string reply001 = MessageBuilder::buildNumericReply(
		m_server_name, RPL_WELCOME, client.getNickname(),
		"Welcome to the Internet Relay Network " + client.getNickname() + "!" +
		client.getUsername() + "@localhost"
	);
	sendReply(client, reply001);

	// RPL_YOURHOST (002): Server information
	std::string reply002 = MessageBuilder::buildNumericReply(
		m_server_name, RPL_YOURHOST, client.getNickname(),
		"Your host is " + m_server_name + ", running version 1.0"
	);
	sendReply(client, reply002);

	// RPL_CREATED (003): Server creation date
	std::string reply003 = MessageBuilder::buildNumericReply(
		m_server_name, RPL_CREATED, client.getNickname(),
		"This server was created 2025-12-21"
	);
	sendReply(client, reply003)
	;
	// RPL_MYINFO (004): Server name, version, and available modes
	std::string reply004 = MessageBuilder::buildNumericReply(
		m_server_name, RPL_MYINFO, client.getNickname(),
		m_server_name + " 1.0 o o"
	);
	sendReply(client, reply004);
}

/**
 * @brief Handle PASS command - authenticate client with server password.
 * Format: PASS <password>
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if client is already registered -> error 462
 * 			2. Check if password parameter exists -> error 461
 * 			3. Extract password from params[0] or trailing
 * 			4. Compare with server password
 * 			5. If match: set client.setAuthenticated(true)
 * 			6. If no match: send error 464 (ERR_PASSWDMISMATCH)
 * 			7. Log authenticated result
 */
void CommandHandler::handlePass(Client& client, const Message& msg) {
	// Check if client already completed registration
	if (client.isRegistered()) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_ALREADYREGISTERED, client.getNickname(), "",
			"You may not reregister"
		);
		sendReply(client, error);
		return;
	}

	// Check if password parameter was provided
	if (msg.params.empty() && msg.trailing.empty()) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NEEDMOREPARAMS, "*", "PASS",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	// Extract password (can be in params[0] or trailing)
	std::string password = msg.params.empty() ? msg.trailing : msg.params[0];

	// Verify password against server password
	if (password == m_password) {
		client.setAuthenticated(true);
		std::cout << "Client fd " << client.getFD() << " authenticated successfully\n";
	} else {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_PASSWDMISMATCH, "*", "",
			"Password incorrect"
		);
		sendReply(client, error);
		std::cout << "Client fd " << client.getFD() << " authentication failed\n";
	}
}

/**
 * @brief Handle NICK command - set or change client's nickname.
 * Format: NICK <nickname>
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if nickname parameter exists -> error431
 * 			2. Extract nickname from params[0] or trailing
 * 			3. Validate nickname format (RFC 1459) -> error 432
 * 			4. Check if nickname already in use -> error 433
 * 			5. Set client's nickname
 * 			6. If already registered: notify about nick change
 * 			7. Check if registration is now complete -> send welcome
 */
void CommandHandler::handleNick(Client& client, const Message& msg) {
	// Check if nickname parameter was provided
	if (msg.params.empty() && msg.trailing.empty()) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NONICKNAMEGIVEN, "*", "",
			"No nickname given"
		);
		sendReply(client, error);
		return;
	}

	// Extract nickname (can be in params[0] or trailing)
	std::string new_nick = msg.params.empty() ? msg.trailing : msg.params[0];

	// Validate nickname according to RFC 1459 rules
	if (!isValidNickname(new_nick)) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_ERRONEOUSNICKNAME, "*", new_nick,
			"Erroneous nickname"
		);
		sendReply(client, error);
		return;
	}

	// Check if nickname is already taken by another client
	if (isNicknameInUse(new_nick, client.getFD())) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NICKNAMEINUSE, "*", new_nick,
			"Nickname is already in use"
		);
		sendReply(client, error);
		return;
	}

	// Store old nickname for notification (if changing nick)
	std::string old_nick = client.getNickname();

	// Set the new nickname
	client.setNickname(new_nick);
	std::cout << "Client fd " << client.getFD() << " nickname set to: " << new_nick << "\n";

	// If client is already registered, notify about nick change
	if (client.isRegistered()) {
		std::string nick_change = ":" + old_nick + "!" + client.getUsername() +
									"@localhost NICK :" + new_nick + "\r\n";
		sendReply(client, nick_change);
	}
	
	// Check if client can now be registered
	// Registration requires: authenticated + nickname + username
	if (!client.isRegistered() && client.isAuthenticated() &&
		!client.getNickname().empty() && !client.getUsername().empty()) 
	{
		client.setRegistered(true);
		sendWelcome(client);
		std::cout << "Client fd " << client.getFD() << " is now fully registered\n";
	}
}

/**
 * @brief Handle USER command - set username and realname.
 * Format: USER <username> <hostname> <servername> :<realname>
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if already registered -> erorr 462
 * 			2. Check parameter count (need 3 params + trailing) -> error 461
 * 			3. Extract username from params[0]
 * 			4. Extract realname from trailing
 * 			5. Set client's username and realname
 * 			6. Check if registration is now complete -> send welcome
 */
void CommandHandler::handleUser(Client& client, const Message& msg) {
	// Check if client already completed registration
	if (client.isRegistered()) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_ALREADYREGISTERED, client.getNickname(), "",
			"You may not reregister"
		);
		sendReply(client, error);
		return;
	}

	// Check parameter count
	// USER command requires: <username> <hostname> <servername> :<realname>
	// We need at least 3 parameters + trailing
	if (msg.params.size() < 3 || msg.trailing.empty())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NEEDMOREPARAMS, "*", "USER",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	// Extract username and realname
	// params[0] = username, params[1] = hostname (ignored),
	// params[2] = servername (ignored), trailing = realname
	client.setUsername(msg.params[0]);
	client.setRealname(msg.trailing);

	std::cout << "Client fd " << client.getFD() << " username: " << msg.params[0]
				<< ", realname: " << msg.trailing << "\n";

	// Check if client can now be registered
	// Registration requires: authenticated + nickname + username
	if (!client.isRegistered() && client.isAuthenticated() &&
		!client.getNickname().empty() && !client.getUsername().empty())
	{
		client.setRegistered(true);
		sendWelcome(client);
		std::cout << "Client fd " << client.getFD() << " is now fully registered\n";
	}
}

/**
 * @brief Handle PING command - respond with PONG to keep connection alive.
 * Format: PING <token> or PING :<token>
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if client is registered -> error 451
 * 			2. Extract token from params[0] or trailing
 * 			3. If token is empty -> use server_name as default
 * 			4. Build PONG response: :<server> PONG <server> :<token>
 * 			5. Send PONG to client
 */
void CommandHandler::handlePing(Client& client, const Message& msg)
{
	// Check if client is registered (must complete PASS/NICK/USER first)
	if (!client.isRegistered()) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTREGISTERED,
			client.getNickname().empty() ? "*" : client.getNickname(),
			"",
			"You have not registered"
		);
		sendReply(client, error);
		return;
	}

	// Extract token (can be in params[0] or trailing)
	std::string token;
	if (!msg.params.empty())
		token = msg.params[0];
	else if (!msg.trailing.empty())
		token = msg.trailing;
	else
		token = m_server_name;	// Default token if none provided
	
	// Build PONG response
	// Format: :<server> PONG <server> :<token>
	std::vector<std::string> pong_params;
	pong_params.push_back(m_server_name);

	std::string pong_reply = MessageBuilder::buildCommandMessage(
		m_server_name, "PONG", pong_params, token
	);
	
	sendReply(client, pong_reply);

	std::cout << "Client fd " << client.getFD() << " PING/PONG: " << token << "\n";
}

/**
 * 
 */
void CommandHandler::handleQuit(Client& client, const Message& msg)
{
	
}

/**
 * @brief Main command dispatcher - routes commands to appropriate handlers.
 * 
 * @param raw_command Complete IRC command with \r\n
 * @param client Client who sent the command
 * 
 * Algorithm:
 * 			1. Try to parse command using Parser::parse()
 * 			2. Log the parsed command
 * 			3. Route based on command name:
 * 				- "PASS" -> handlePass()
 * 				- "NICK" -> handleNick()
 * 				- "USER" -> handleUser()
 * 				- unknown -> ERR_UNKNOWNCOMMAND (421)
 * 			4. Catch parsing errors and log them
 */
void CommandHandler::handleCommand(const std::string& raw_command, Client& client) {
	try {
		// Parse the raw IRC message into structured format
		Message msg = Parser::parse(raw_command);

		std::cout << "Parsed command: " << msg.command << " from fd " << client.getFD() << "\n";

		// Route to appropriate command handler
		if (msg.command == "PASS")
			handlePass(client, msg);
		else if (msg.command == "NICK")
			handleNick(client, msg);
		else if (msg.command == "USER")
			handleUser(client, msg);
		else if (msg.command == "PING")
			handlePing(client, msg);
		else {
			// Command not recognized or not implemented
			std::string error = MessageBuilder::buildErrorReply(
				m_server_name, ERR_UNKNOWNCOMMAND,
				client.getNickname().empty() ? "*" : client.getNickname(),
				msg.command, "Unknown command"
			);
			sendReply(client, error);
		}
	} catch (const std::exception& e) {
		// Log parsing errors but don't crash the server
		std::cerr << "Error parsing command from fd " << client.getFD()
				<< ": " << e.what() << "\n";
	}
}
