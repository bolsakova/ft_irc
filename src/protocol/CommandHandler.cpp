/**
 * CommandHandler.cpp
 * 
 * IRC command dispatcher and handler implementation.
 * Routes parsed messages to appropriate command handlers (PASS, NICK, USER,
 * PING, QUIT, PRIVMSG, JOIN, PART, KICK, INVITE, TOPIC, MODE).
 * 
 * RFC 1459: https://tools.ietf.org/html/rfc1459
 */

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
 * @brief Helper function to append a reply to client's output buffer.
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
 * Validate nickname according to RFC 1459 rules.
 * 
 * @param nickname Nickname string to validate
 * @return true if valid, false otherwise
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
 * Check if a nickname is already in use by another client
 * Used to prevent duplicate nicknames on the server
 * 
 * @param nickname Nickname to check
 * @param exclude_fd File descriptor to exclude (for nick changes)
 * @return true if nickname is taken, false if available
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
 * Validate channel name according to IRC rules
 * 
 * @param name Channel name to validate
 * @return True if valid, false otherwise
 * 
 * IRC channel name rules:
 * 		- Starts with # or &
 * 		- Length: 1-50 characters (including #)
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
 * Send welcome messages (RPL_WELCOME through RPL_MYINFO) to client.
 * Called after successful registration (PASS + NICK + USER complete)
 * 
 * @param client Newly registered client
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
 * Handle PASS command - authenticate client with server password.
 * Format: PASS <password>
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
 * Handle NICK command - set or change client's nickname.
 * Format: NICK <nickname>
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
 * Handle USER command - set username and realname.
 * Format: USER <username> <hostname> <servername> :<realname>
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
 * Handle PING command - respond with PONG to keep connection alive.
 * Format: PING <token> or PING :<token>
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
 * Handle PRIVMSG command - send private message to user or channel.
 * Format: PRIVMSG <target> :<message>
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
 * Handle JOIN command - join or create a channel
 * Format: JOIN <channel> [key]
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
 * Handle PART command - client leaves a channel
 * Syntax: PART <channel> [<reason>]
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
 * Handle KICK command - operator removes user from channel
 * Format: KICK <channel> <user> [<reason>]
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
	Client* target_client = nullptr;
	int target_fd = -1;
	
	const std::map<int, Client*>& members = chan->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin();
		it != members.end(); ++it)
	{
		if (it->second->getNickname() == target_nick)
		{
			target_client = it->second;
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
 * Handle INVITE command - invite user to channel
 * Format: INVITE <nickname> <channel>
 */
void CommandHandler::handleInvite(Client& client, const Message& msg) {
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

	// Check if both channel and nickname parameters are provided
	if (msg.params.size() < 2)
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NEEDMOREPARAMS,
			client.getNickname(),
			"INVITE",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	std::string target_nick = msg.params[0];
	std::string channel_name = msg.params[1];

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

	// Check if sender is operator (required for +i channels)
	if (chan->isInviteOnly() && !chan->isOperator(client.getFD()))
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

	// Find target user by nickname - search in channel members first
	Client* target_client = nullptr;
	int target_fd = -1;
	
	const std::map<int, Client*>& members = chan->getMembers();
	for (std::map<int, Client*>::const_iterator it = members.begin();
		it != members.end(); ++it)
	{
		if (it->second->getNickname() == target_nick)
		{
			target_client = it->second;
			target_fd = it->first;
			break;
		}
	}

	// If not found in channel, search in all clients
	if (!target_client)
	{
		target_client = m_server.findClientByNickname(target_nick);
		if (target_client)
			target_fd = target_client->getFD();
	}

	// Check if target user exists
	if (!target_client)
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOSUCHNICK,
			client.getNickname(),
			target_nick,
			"no such nick/channel"
		);
		sendReply(client, error);
		return;
	}

	// Check if target is already on the channel
	if (chan->isMember(target_fd))
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_USERONCHANNEL,
			client.getNickname(),
			target_nick + " " + channel_name,
			"is already on channel"
		);
		sendReply(client, error);
		return;
	}

	// Add target to invited list
	chan->addInvited(target_fd);

	// Build INVITE message for target
	// Format: :sender!user@host INVITE target :#channel
	std::string prefix = client.getNickname() + "!" +
						client.getUsername() + "@localhost";
	std::vector<std::string> params;
	params.push_back(target_nick);
	
	std::string invite_msg = MessageBuilder::buildCommandMessage(
		prefix, "INVITE", params, channel_name
	);
	
	// Send INVITE to target user
	sendReply(*target_client, invite_msg);
	
	// Send RPL_INVITING (341) to sender
    // Format: :server 341 sender target #channel
	std::string inviting_reply = MessageBuilder::buildNumericReply(
		m_server_name, RPL_INVITING, client.getNickname(),
		target_nick + " " + channel_name
	);
	sendReply(client, inviting_reply);

	std::cout << client.getNickname() << " invited " << target_nick
				<< " to " << channel_name << "\n";
}

/**
 * Handle TOPIC command - view or change channel topic
 * Format: TOPIC <channel> [:<new topic>]
 */
void CommandHandler::handleTopic(Client& client, const Message& msg) {
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
			"TOPIC",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	std::string channel_name = msg.params[0];

	// Find the channel
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
	
	// Check if this is a query (view topic) or set (change topic)
	if (msg.trailing.empty())
	{
		// Query - show current topic
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
	else
	{
		// Check if channel is +t (topic protected)
		if (chan->isTopicProtected() && !chan->isOperator(client.getFD()))
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

		// Set the new topic
		std::string new_topic = msg.trailing;
		chan->setTopic(new_topic);

		// Build TOPIC message
		// Format: :nick!user@host TOPIC #channel :new topic
		std::string prefix = client.getNickname() + "!" +
							client.getUsername() + "@localhost";
		std::vector<std::string> params;
		params.push_back(channel_name);
		
		std::string topic_msg = MessageBuilder::buildCommandMessage(
			prefix, "TOPIC", params, new_topic
		);

		// Broadcast TOPIC change to all channel members
		chan->broadcast(topic_msg);

		std::cout << client.getNickname() << " set topic for "
					<< channel_name << ": " << new_topic << "\n";
	}
}

/**
 * Handle MODE command - view or change channel modes
 * Format: MODE <channel> [<+/-modes> [parameters...]]
 */
void CommandHandler::handleMode(Client& client, const Message& msg) {
	// Check if client is registered
	if (!client.isRegistered())
	{
		std::string error = MessageBuilder::buildErrorReply(
			m_server_name, ERR_NOTREGISTERED,
			client.getNickname().empty() ? "*" : client.getNickname(),
			"",
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
			"MODE",
			"Not enough parameters"
		);
		sendReply(client, error);
		return;
	}

	std::string channel_name = msg.params[0];

	// Validate channel exists
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

	// MODE viewing (no mode string provided)
	if (msg.params.size() == 1 && msg.trailing.empty())
	{
		// Build current mode string from channel state
		std::string modes = "+";
		std::string mode_params;

		if (chan->isInviteOnly())
			modes += 'i';
		if (chan->isTopicProtected())
			modes += 't';
		if (chan->hasKey())
		{
			modes += 'k';
			mode_params += " " + chan->getKey();
		}
		if (chan->getUserLimit() > 0)
		{
			modes += 'l';
			mode_params += " ";

			// Convert int to string manually
			std::ostringstream oss;
			oss << chan->getUserLimit();
			mode_params += oss.str();
		}

		// If no modes set, just send "+"
		if (modes == "+")
			modes = "+";
		
		// Send RPL_CHANNELMODEIS (324)
		std::string reply = ":" + m_server_name + " 324 " +
							client.getNickname() + " " +
							channel_name + " " + modes + mode_params + "\r\n";
		sendReply(client, reply);
		return;
	}

	// MODE changing (mode string provided)
	// Check if sender is operator
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

	std::string mode_string = msg.params[1];
	char action = '+';		// Current action (+ or -)
	size_t param_index = 2;	// Start at msg.params[2] for mode parameters
	
	std::string applied_modes;					// Track applied mode changes
	std::vector<std::string> applied_params;	// Track parameters for applied modes
	char current_action = '\0';					// Track last added action to applied_modes

	// Parse mode string character by character
	for (size_t i = 0; i < mode_string.length(); ++i)
	{
		char c = mode_string[i];

		// Handle action switches
		if (c == '+' || c == '-')
		{
			action = c;
			continue;
		}

		// Process mode character
		switch (c)
		{
			case 'i':
			{
				// Invite-only mode
				if (action == '+')
					chan->setInviteOnly(true);
				else
					chan->setInviteOnly(false);
				
				// Add to applied modes string
				if (current_action != action)
				{
					applied_modes += action;
					current_action = action;
				}
				applied_modes += 'i';
				break;
			}
			case 't':
			{
				// Topic protection mode
				if (action == '+')
					chan->setTopicProtected(true);
				else
					chan->setTopicProtected(false);
				
				// Add to applied modes string
				if (current_action != action)
				{
					applied_modes += action;
					current_action = action;
				}
				applied_modes += 't';
				break;
			}
			case 'k':
			{
				// Channel key mode
				if (action == '+')
				{
					// Need key parameter
					if (param_index >= msg.params.size())
					{
						std::string error = MessageBuilder::buildErrorReply(
							m_server_name, ERR_NEEDMOREPARAMS,
							client.getNickname(),
							"MODE",
							"Not enough parameters"
						);
						sendReply(client, error);
						continue;
					}
					std::string key = msg.params[param_index++];
					chan->setKey(key);

					// Add to applied modes string
					if (current_action != action)
					{
						applied_modes += action;
						current_action = action;
					}
					applied_modes += 'k';
					applied_params.push_back(key);
				}
				else
				{
					// Remove key
					chan->removeKey();

					// Add to applied modes string
					if (current_action != action)
					{
						applied_modes += action;
						current_action = action;
					}
					applied_modes += 'k';
				}
				break;
			}
			case 'o':
			{
				// Operator privileges mode
				// Need nickname parameter for both + and -
				if (param_index >= msg.params.size())
				{
					std::string error = MessageBuilder::buildErrorReply(
						m_server_name, ERR_NEEDMOREPARAMS,
						client.getNickname(),
						"MODE",
						"Not enough parameters"
					);
					sendReply(client, error);
					continue;
				}
				
				std::string target_nick = msg.params[param_index++];
				
				// Find target user by nickname
				Client* target = m_server.findClientByNickname(target_nick);
				if (!target)
				{
					std::string error = MessageBuilder::buildErrorReply(
						m_server_name, ERR_NOSUCHNICK,
						client.getNickname(),
						target_nick,
						"No such nick/channel"
					);
					sendReply(client, error);
					continue;

				}

				// Check if target is on the channel
				if (!chan->isMember(target->getFD()))
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

				// Apply operator change
				if (action == '+')
					chan->addOperator(target->getFD());
				else
					chan->removeOperator(target->getFD());
				
				// Add to applied modes string
				if (current_action != action)
				{
					applied_modes += action;
					current_action = action;
				}
				applied_modes += 'o';
				applied_params.push_back(target_nick);
				break;
			}
			case 'l':
			{
				// User limit mode
				if (action == '+')
				{
					// Need limit parameter
					if (param_index >= msg.params.size())
					{
						std::string error = MessageBuilder::buildErrorReply(
							m_server_name, ERR_NEEDMOREPARAMS,
							client.getNickname(),
							"MODE",
							"Not enough parameters"
						);
						sendReply(client, error);
						continue;
					}

					std::string limit_str = msg.params[param_index++];

					// Convert string to int
					int limit = 0;
					std::istringstream iss(limit_str);
					if (!(iss >> limit) || limit <= 0)
					{
						// Invalid limit - ignore
						continue;
					}

					chan->setUserLimit(limit);

					// Add to applied modes string
					if (current_action != action)
					{
						applied_modes += action;
						current_action = action;
					}
					applied_modes += 'l';
					applied_params.push_back(limit_str);
				}
				else
				{
					// Remove limit
					chan->setUserLimit(0);

					// Add to applied modes string
					if (current_action != action)
					{
						applied_modes += action;
						current_action = action;
					}
					applied_modes += 'l';
				}
				break;
			}
			
			default:
			{
				// Unknown mode - send error
				std::string unknown_mode(1, c);
				std::string error = MessageBuilder::buildErrorReply(
					m_server_name, ERR_UNKNOWNMODE,
					client.getNickname(),
					unknown_mode,
					"is unknown mode char to me"
				);
				sendReply(client, error);
				break;
			}
		}
	}
	
	// If any modes were applied, broadcast the change
	if (!applied_modes.empty())
	{
		std::string prefix = client.getNickname() + "!" +
							client.getUsername() + "@localhost";
		std::vector<std::string> params;
		params.push_back(channel_name);
		params.push_back(applied_modes);

		// Add mode parameters
		for (size_t i = 0; i < applied_params.size(); ++i)
			params.push_back(applied_params[i]);
		
		std::string mode_msg = MessageBuilder::buildCommandMessage(
			prefix, "MODE", params, ""
		);
		
		// Broadcast to all channel members (including sender)
		chan->broadcast(mode_msg);
	}
}

/**
 * @brief Main command dispatcher - routes commands to appropriate handlers.
 * @param raw_command Complete IRC command with \r\n
 * @param client Client who sent the command
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
		else if (msg.command == "INVITE")
			handleInvite(client, msg);
		else if (msg.command == "TOPIC")
			handleTopic(client, msg);
		else if (msg.command == "MODE")
			handleMode(client, msg);
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
