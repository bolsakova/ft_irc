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
 * @brief Validate channel name according to IRC rules
 * 
 * @param name Channel name to validate
 * @return True if valid, false otherwise
 * 
 * IRC channel name rules:
 * 		- Starts with # or &
 * 		- Length: 1-50 characters (including #)
 * 		- Cannot contain: space, comma, control-G (bell)
 */
bool CommandHandler::isValidChannelName(const std::string& name) {
	// Check minimum length (at least # + 1 char)
	if (name.length() < 2)
		return false;

	// Check maximum length
	if (name.length() > 50)
		return false;
	
	// Must start with # or &
	if (name[0] != '#' && name[0] != '&')
		return false;
	
	// Check for forbidden characters
	for (size_t i = 0; i < name.length(); ++i)
	{
		char c = name[i];
		// Space, comma, or control-G (bell, ASCII 7)
		if (c == ' ' || c == ',' || c == '\a' || c == '\0')
			return false;
	}
	
	return true;
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
 * @brief Handle QUIT command - disconnect cliet from server.
 * Format: QUIT [:<reason>]
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Extract reason from trailing (or use default "Client exited")
 * 			2. Build QUIT message with client prefix
 * 			3. Find all channels where client is a member
 * 			4. Broadcast QUIT to all channel members
 * 			5. Remove client from all channels
 * 			6. Mark client for disconnection
 * 			7. Log quit action
 * 
 * @note Client can use QUIT even without registration
 */
void CommandHandler::handleQuit(Client& client, const Message& msg)
{
	// Extract quit reason (optional)
	std::string reason = msg.trailing.empty() ? "Client exited" : msg.trailing;

	std::cout << "Client fd " << client.getFD() << " quit: " << reason << "\n";

	// Build QUIT message to broadcast to channels
	// Format: :nick!user@host QUIT :reason
	std::string quit_msg;
	if (client.isRegistered()) {
		std::string prefix = client.getNickname() + "!" +
							client.getUsername() + "@localhost";
		std::vector<std::string> empty_params;
		quit_msg = MessageBuilder::buildCommandMessage(
			prefix, "QUIT", empty_params, reason
		);

		// Get all channels and broadcast QUIT to channels where client is a member
		const std::map<std::string, std::unique_ptr<Channel>>& channels = m_server.getChannels();

		for (std::map<std::string, std::unique_ptr<Channel>>::const_iterator it = channels.begin();
			it != channels.end(); ++it)
		{
			Channel* chan = it->second.get();
			if (chan && chan->isMember(client.getFD())) {
				// Broadcast QUIT to all members of this channel
				chan->broadcast(quit_msg);
				// Remove client from channel
				chan->removeMember(client.getFD());
			}
		}
	}
	
	// Mark client for disconnection
	// Server will handle actual disconnection in main loop
	client.markForDisconnect(reason);
}

/**
 * @brief Handle PRIVMSG command - send private message to user or channel.
 * Format: PRIVMSG <target> :<message>
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if client is registered -> error 451
 * 			2. Check if target parameter exists -> error 411
 * 			3. Check if message text exists -> 412
 * 			4. Determine target type:
 * 				- If starts with # -> channel
 * 				- Otherwise -> user nickname
 * 			5. For channel:
 * 				- Check channel exists -> error 403
 * 				- Check sender is member -> error 404
 * 				- Broadcast to all members except sender
 * 			6. For user:
 * 				- Find target user by nickname -> error 401
 * 				- Send message to target
 */
void CommandHandler::handlePrivmsg(Client& client, const Message& msg)
{
	// Check if client registered
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

	// Check if target parameter was provided
	if (msg.params.empty())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NORECIPIENT,
			client.getNickname(),
			"",
			"No recipient given (PRIVMSG)"
		);
		sendReply(client, error);
		return;
	}

	// Check if message text was provided
	if (msg.trailing.empty())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTEXTTOSEND,
			client.getNickname(),
			"",
			"No text to send"
		);
		sendReply(client, error);
		return;
	}

	std::string target = msg.params[0];
	std::string message = msg.trailing;

	// Check if target is a channel (starts with #)
	if (!target.empty() && target[0] == '#')
	{
		// Find channel
		Channel* chan = m_server.findChannel(target);

		if (!chan)
		{
			// Channel doesn't exist
			std::string error = MessageBuilder::buildErrorReply(
				m_server_name, ERR_NOSUCHCHANNEL,
				client.getNickname(),
				target,
				"No such channel"
			);
			sendReply(client, error);
			return;
		}

		// Check if sender is a member of the channel
		if (!chan->isMember(client.getFD()))
		{
			std::string error = MessageBuilder::buildErrorReply(
				m_server_name, ERR_CANNOTSENDTOCHAN,
				client.getNickname(),
				target,
				"Cannot send to channel"
			);
			sendReply(client, error);
			return;
		}

		// Build PRIVMSG for channel
		// Format: :sender!user@host PRIVMSG #channel :message
		std::string prefix = client.getNickname() + "!" +
							client.getUsername() + "@localhost";
		std::vector<std::string> params;
		params.push_back(target);

		std::string privmsg = MessageBuilder::buildCommandMessage(
			prefix, "PRIVMSG", params, message
		);

		// Broadcast to all channel memers except sender
		chan->broadcast(privmsg, client.getFD());

		std::cout << "PRIVMSG from " << client.getNickname()
					<< " to channel " << target << ": " << message << "\n";
	} 
	else
	{
		// Target is a user nickname - find the user
		const std::map<int, std::unique_ptr<Client>>& clients = m_server.getClients();
		Client* target_client = nullptr;

		for (std::map<int, std::unique_ptr<Client>>::const_iterator it = clients.begin();
			it != clients.end(); ++it)
		{
			if (it->second->getNickname() == target)
			{
				target_client = it->second.get();
				break;
			}
		}

		// Check if target user exists
		if (!target_client)
		{
			std::string error = MessageBuilder::buildErrorReply(
				m_server_name, ERR_NOSUCHNICK,
				client.getNickname(),
				target,
				"No such nick/channel"
			);
			sendReply(client, error);
			return;
		}

		// Build PRIVMSG with sender prefix
		// Format: :sender!user@host PRIVMSG target :message
		std::string prefix = client.getNickname() + "!" +
							client.getUsername() + "@localhost";
		
		std::vector<std::string> params;
		params.push_back(target);

		std::string privmsg = MessageBuilder::buildCommandMessage(
			prefix, "PRIVMSG", params, message
		);

		// Send to target user
		sendReply(*target_client, privmsg);

		std::cout << "PRIVMSG from " << client.getNickname()
					<< " to " << target << ": " << message << "\n";
	}
}

/**
 * @brief Handle JOIN command - join or create a channel
 * Format: JOIN <channel> [key]
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if client is registered -> error 451
 * 			2. Check if channel parameter exists -> error 461
 * 			3. Extract channel name and optional key
 * 			4. Validate channel name format
 * 			5. Find or create channel:
 * 				- If not exists: create and make client operator
 * 				- If exists: check modes (+i, +k, +l)
 * 			6. Add client to channel
 * 			7. Send JOIN confirmation to client
 * 			8. Broadcast JOIN to all channel members
 * 			9. Send NAMES list (RPL_NAMREPLY + RPL_ENDOFNAMES)
 * 			10. Send TOPIC if set (RPL_TOPIC or RPL_NOTOPIC)
 */
void CommandHandler::handleJoin(Client& client, const Message& msg) {
	// Check if client is registered
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

	// Check if channel parameter was provided
	if (msg.params.empty())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NEEDMOREPARAMS,
			client.getNickname(),
			"JOIN",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	std::string channel_name = msg.params[0];
	std::string key = (msg.params.size() > 1) ? msg.params[1] : "";

	// Validate channel name
	if (!isValidChannelName(channel_name)) {
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOSUCHCHANNEL,
			client.getNickname(),
			channel_name,
			"Invalid channel name"
		);
		sendReply(client, error);
		return;
	}

	// Find or create channel
	Channel* chan = m_server.findChannel(channel_name);
	bool is_new_channel = (chan == nullptr);

	if (is_new_channel)
	{
		// Create new channel
		chan = m_server.createChannel(channel_name);
		// First member becomes operator
		chan->addOperator(client.getFD());
		std::cout << "Created new channel: " << channel_name << ", operator: " << client.getNickname() << "\n";
	}
	else
	{
		// Channel exists - check modes
		// Check +i (invite-only)
		if (chan->isInviteOnly())
		{
			if (!chan->isInvited(client.getFD()))
			{
				std::string error = MessageBuilder::buildErrorReply(
					m_server_name, ERR_INVITEONLYCHAN,
					client.getNickname(),
					channel_name,
					"Cannot join channel (+i)"
				);
				sendReply(client, error);
				return;
			}
			// Remove from invited list after successful join
			chan->removeInvited(client.getFD());
		}

		// Check +k (channel key)
		if (chan->hasKey())
		{
			if (key != chan->getKey())
			{
				std::string error = MessageBuilder::buildErrorReply(
					m_server_name, ERR_BADCHANNELKEY,
					client.getNickname(),
					channel_name,
					"Cannot join channel (+k)"
				);
				sendReply(client, error);
				return;
			}
		}
		
		// Check +l (user limit)
		int limit = chan->getUserLimit();
		if (limit > 0 && static_cast<int>(chan->getMembers().size()) >= limit)
		{
			std::string error = MessageBuilder::buildErrorReply(
				m_server_name, ERR_CHANNELISFULL,
				client.getNickname(),
				channel_name,
				"Cannot join channel (+l)"
			);
			sendReply(client, error);
			return;
		}
	}

	// Add client to channel
	chan->addMember(&client);

	// Build JOIN message
	// Format: :nick!user@host JOIN :#channel
	std::string prefix = client.getNickname() + "!" +
						client.getUsername() + "@localhost";
	std::vector<std::string> empty_params;
	std::string join_msg = MessageBuilder::buildCommandMessage(
		prefix, "JOIN", empty_params, channel_name
	);

	// Broadcast JOIN to all members (including sender)
	chan->broadcast(join_msg);

	std::cout << client.getNickname() << " joined " << channel_name << "\n";

	// Send NAMES list (RPL_NAMREPLY + RPL_ENDOFNAMES)
	// Build list of nicknames with @ prefix for operators
	std::string names_list;
	const std::map<int, Client*>& members = chan->getMembers();

	for (std::map<int, Client*>::const_iterator it = members.begin();
		it != members.end(); ++it)
	{
		if (it != members.begin())
			names_list += " ";
		
		// Add @ prefix for operators
		if (chan->isOperator(it->first))
			names_list += "@";
		
		names_list += it->second->getNickname();
	}

	// RPL_NAMREPLY (353): :server 353 nick = #channel :names
	std::string names_reply = ":" + m_server_name + " 353 " +
								client.getNickname() + " = " +
								channel_name + " :" + names_list + "\r\n";
	sendReply(client, names_reply);

	// RPL_ENDOFNAMES (366): :server 366 nick #channel :End of /NAMES list
	std::string end_names = MessageBuilder::buildNumericReply(
		m_server_name, RPL_ENDOFNAMES, client.getNickname(),
		channel_name + " :End of /NAMES list"
	);
	sendReply(client, end_names);

	// Send TOPIC if set
	if (chan->hasTopic())
	{
		// RPL_TOPIC (332)
		std::string topic_reply = ":" + m_server_name + " 332 " +
								client.getNickname() + " " +
								channel_name + " :" + chan->getTopic() + "\r\n";
		sendReply(client, topic_reply);
	}
	else
	{
		// RPL_NOTOPIC (331)
		std::string no_topic = MessageBuilder::buildNumericReply(
			m_server_name, RPL_NOTOPIC, client.getNickname(),
			channel_name + " :No topic is set"
		);
		sendReply(client, no_topic);
	}
}

/**
 * @brief Handle PART command - client leaves a channel
 * Syntax: PART <channel> [<reason>]
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 		1. Check if client is registered
 * 		2. Verify channel parameter is provided
 * 		3. Validate channel exists
 * 		4. Check if client is a member of the channel
 * 		5. Remove client from channel
 * 		6. Broadcast PART message to all channel members (including sender)
 * 		7. Optionally include reason for leaving
 * 
 * Responses:
 * 		- ERR_NEEDMOREPARAMS (461): No channel specified
 * 		- ERR_NOSUCHCHANNEL (403): Channel doesn't exist
 * 		- ERR_NOTONCHANNEL (442): Client is not on that channel
 * 		- Success: :nick!user@host PART #channel [:reason]
 */
void CommandHandler::handlePart(Client& client, const Message& msg) {
	// Check if client is registered
	if (!client.isRegistered())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTREGISTERED,
			"*", "",
			"You have not registered"
		);
		sendReply(client, error);
		return;
	}

	// Check if channel parameter is provided
	if (msg.params.empty())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NEEDMOREPARAMS,
			client.getNickname(),
			"PART",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	std::string channel_name = msg.params[0];
	std::string reason = msg.trailing;

	// Find channel
	Channel* chan = m_server.findChannel(channel_name);

	if (!chan)
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOSUCHCHANNEL,
			client.getNickname(),
			channel_name,
			"No such channel"
		);
		sendReply(client, error);
		return;
	}

	// Check if client is a member of the channel
	if (!chan->isMember(client.getFD()))
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTONCHANNEL,
			client.getNickname(),
			channel_name,
			"You're not on that channel"
		);
		sendReply(client, error);
		return;
	}

	// Build PART message
	// Format: :nick!user@host PART #channel [:reason]
	std::string prefix = client.getNickname() + "!" +
						client.getUsername() + "@localhost";
	std::vector<std::string> params;
	params.push_back(channel_name);
	
	std::string part_msg = MessageBuilder::buildCommandMessage(
		prefix, "PART", params, reason
	);

	// Broadcast PART to all channel members (including sender)
	chan->broadcast(part_msg);

	std::cout << client.getNickname() << " left " << channel_name;
	if (!reason.empty())
		std::cout << " (" << reason << ")";
	std::cout << "\n";

	// Remove client from channel
	chan->removeMember(client.getFD());
}

/**
 * @brief Handle KICK command - operator removes user from channel
 * Format: KICK <channel> <user> [<reason>]
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 		1. Check if client is registered
 * 		2. Verify channel and user parameters are provided
 * 		3. Validate channel exists
 * 		4. Check if sender is on the channel
 * 		5. Check if sender is a channel operator
 * 		6. Find target user by nickname
 * 		7. Check if target is on the channel
 * 		8. Remove target from channel
 * 		9. Broadcast KICK message to all channel members (including kicked user)
 * 		10. Optionally include reason
 * 
 * Responses:
 * 		- ERR_NEEDMOREPARAMS (461): Missing channel or user parameter
 * 		- ERR_NOSUCHCHANNEL (403): Channel doesn't exist
 * 		- ERR_NOTONCHANNEL (442): Sender is not on that channel
 * 		- ERR_CHANOPRIVSNEEDED (482): Sender is not a channel operator
 * 		- ERR_USERNOTINCHANNEL (441): Target user is not on channel
 * 		- Success: :operator!user@host KICK #channel target [:reason]
 */
void CommandHandler::handleKick(Client& client, const Message& msg) {
	// Check if client is registered
	if (!client.isRegistered())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTREGISTERED,
			"*", "",
			"You have not registered"
		);
		sendReply(client, error);
		return;
	}

	// Check if both channel and user parameters are provided
	if (msg.params.size() < 2)
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NEEDMOREPARAMS,
			client.getNickname(),
			"KICK",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	std::string channel_name = msg.params[0];
	std::string target_nick = msg.params[1];
	std::string reason = msg.trailing.empty() ? client.getNickname() : msg.trailing;

	// Find channel
	Channel* chan = m_server.findChannel(channel_name);

	if (!chan)
	{
		// Channel doesn't exist
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOSUCHCHANNEL,
			client.getNickname(),
			channel_name,
			"No such channel"
		);
		sendReply(client, error);
		return;
	}

	// Check if sender is a member of the channel
	if (!chan->isMember(client.getFD()))
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTONCHANNEL,
			client.getNickname(),
			channel_name,
			"You're not on that channel"
		);
		sendReply(client, error);
		return;
	}

	// Check if sender is a channel operator
	if (!chan->isOperator(client.getFD()))
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_CHANOPRIVSNEEDED,
			client.getNickname(),
			channel_name,
			"You're not channel operator"
		);
		sendReply(client, error);
		return;
	}

	// Find target user by nickname
	const std::map<int, std::unique_ptr<Client>>& clients = m_server.getClients();
	Client* target_client = nullptr;
	int target_fd = -1;

	for (std::map<int, std::unique_ptr<Client>>::const_iterator it = clients.begin();
		it != clients.end(); ++it)
	{
		if (it->second->getNickname() == target_nick)
		{
			target_client = it->second.get();
			target_fd = it->first;
			break;
		}
	}

	// Check if target exists and is on the channel
	if (!target_client || !chan->isMember(target_fd))
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_USERNOTINCHANNEL,
			client.getNickname(),
			target_nick + " " + channel_name,
			"They aren't on that channel"
		);
		sendReply(client, error);
		return;
	}

	// Build KICK message
	// Format: :operator!user@host KICK #channel target :reason
	std::string prefix = client.getNickname() + "!" +
						client.getUsername() + "@localhost";
	std::vector<std::string> params;
	params.push_back(channel_name);
	params.push_back(target_nick);
	
	std::string kick_msg = MessageBuilder::buildCommandMessage(
		prefix, "KICK", params, reason
	);

	// Broadcast KICK to all channel members (including kicked user)
	chan->broadcast(kick_msg);

	std::cout << client.getNickname() << " kicked " << target_nick
				<< " from " << channel_name << " (" << reason << ")\n";

	// Remove target from channel
	chan->removeMember(target_fd);
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
		else if (msg.command == "QUIT")
			handleQuit(client, msg);
		else if (msg.command == "PRIVMSG")
			handlePrivmsg(client, msg);
		else if (msg.command == "JOIN")
			handleJoin(client, msg);
		else if (msg.command == "PART")
			handlePart(client, msg);
		else if (msg.command == "KICK")
			handleKick(client, msg);
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
