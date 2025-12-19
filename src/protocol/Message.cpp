#include "../../inc/protocol/Message.hpp"

/**
 * @brief Check if the message has a prefix
 * @return True - if prefix is not empty, False - otherwise
 */
bool Message::hasPrefix() const {
	return !prefix.empty();
}

/**
 * @brief Check if the message has a trailing parameter
 * @return True - if trailing is not empty, False - otherwise
 */
bool Message::hasTrailing() const {
	return !trailing.empty();
}

/**
 * @brief Get the total number of parameters (including trailing if present)
 * @return Number of parameters
 */
size_t Message::getTotalParams() const {
	return params.size() + (hasTrailing() ? 1 : 0);
}

/**
 * @brief Format numeric code to 3-digit string with leading zeros
 * 
 * @param code Numeric code to format (1-999)
 * @return Three-digit string (e.g., 1 -> "001", 99 -> "099")
 * 
 * IRC protocol requires numeric replies to be exactly 3 digits
 */
std::string MessageBuilder::formatCode(int code) {
	std::ostringstream oss;

	// Set width to 3 characters and fill with leading zeros
	oss << std::setw(3) << std::setfill('0') << code;

	return oss.str();
}

/**
 * @brief Validate that IRC message doesn't exceed maximum allowed length
 * 
 * @param message Complete message to validate
 * @throws std::length_error if message exceeds 512 characters
 * 
 * RFC 1459 specifies maximum message length of 512 characters
 * including trailing \r\n
 */
void MessageBuilder::validateLength(const std::string& message) {
	// Maximum IRC message length (including \r\n)
	const size_t MAX_MESSAGE_LENGTH = 512;

	if (message.length() > MAX_MESSAGE_LENGTH)
		throw std::length_error("IRC message exceeds maximum length of 512 characters");
}

/**
 * @brief Build numeric reply message from server to client
 * 
 * @param server Server name sending the reply
 * @param code Numeric reply code (1-999)
 * @param target Tartget nickname (or "*" if unknown yet)
 * @param message Message text content
 * @return Formatted IRC message string ending with \r\n
 * 
 * Format: :<server> <code> <target> :<message>\r\n
 * Example: ":ircserv 001 tanja :Welcome to the IRC Network\r\n"
 * 
 * Used for server replies:
 * - RPL_WELCOME (001) - welcome message
 * - RPL_YOURHOST (002) - host information
 * - RPL_CREATED (003) - server creation date
 * - RPL_MYINFO (004) - server information
 */
std::string MessageBuilder::buildNumericReply(const std::string& server, int code, const std::string& target, const std::string& message) {
	std::string result;

	// Start with prefix (server name)
	result += ':';
	result += server;
	result += ' ';

	// Add formatted numeric code (e.g., "001")
	result += formatCode(code);
	result += ' ';

	// Add target nickname
	result += target;

	// Add trailing parameter with message
	result += " :";
	result += message;

	// Add IRC message terminator
	result += "\r\n";

	// Validate total length doesn't exceed 512 characters
	validateLength(result);

	return (result);
}

/**
 * Build error reply message from server to client
 * 
 * @param server Server name sending the error
 * @param code Error code (400-599)
 * @param target Target nickname (or "*" if unknown yet)
 * @param param Additional parameter indicating error context
 * 				(e.g., problematic nickname, channel name, or command)
 * @param message Error message text
 * @return Formatted IRC error message string ending with \r\n
 * 
 * Format: :<server> <code> <target> <param> :<message>\r\n
 * Example: ":ircserv 433 * tanja :Nickname is already in use\r\n"
 * 
 * Used for error replies:
 * - ERR_NOSUCHNICK (401) - no such nickname exists
 * - ERR_NOSUCHCHANNEL (403) - no such channel exists
 * - ERR_NICKNAMEINUSE (433) - nickname is already taken
 * - ERR_NEEDMOREPARAMS (461) - command requires more parameters
 */
std::string MessageBuilder::buildError(const std::string& server, int code, const std::string& target, const std::string& param, const std::string& message) {
	std::string result;

	// Start with prefix (server name)
	result += ':';
	result += server;
	result += ' ';

	// Add formatted error code (e.g., "433")
	result += formatCode(code);
	result += ' ';

	// Add target nickname
	result += target;
	result += ' ';

	// Add additional parameter (problematic nick/channel/command)
	result += param;

	// Add trailing parameter with error message
	result += " :";
	result += message;

	// Add IRC message terminator
	result += "\r\n";

	// Validate total length doesn't exceed 512 characters
	validateLength(result);

	return result;
}

/**
 * @brief Build command message from server (relay between clients)
 * 
 * @param prefix Message source in format nick!user@host or servername
 * @param command IRC command (JOIN, PART, PRIVMSG, KICK, MODE, etc.)
 * @param params Vector of regular parameters (without spaces)
 * @param trailing Optional trailing parameter (can contain spaces)
 * @return Formatted IRC command message string ending with \r\n
 * 
 * Format: :<prefix> <command> [params...] [:<trailing>]\r\n
 * 
 * Examples:
 * - ":alice!user@host PRIVMSG bob :Hello there!\r\n"
 * - ":alice!user@host JOIN #channel\r\n"
 * - ":alice!user@host MODE #channel +o bob\r\n"
 * - ":alice!user@host KICK #channel bob :Bad behavior\r\n"
 * 
 * Used when server relays messages between clients or sends
 * notifications about channel/user events
 */
std::string MessageBuilder::buildCommand(const std::string& prefix, const std::string& command, const std::vector<std::string>& params, const std::string& trailing) {
	std::string result;

	// Start with prefix (source of the message)
	result += ':';
	result += prefix;
	result += ' ';

	// Add command
	result += command;

	// Add all regular parameters (separated by spaces)
	for (size_t i = 0; i < params.size(); ++i) {
		result += ' ';
		result += params[i];
	}

	// Add trailing parametr if present
	if (!trailing.empty()) {
		result += " :";
		result += trailing;
	}

	// Add IRC message terminator
	result += "\r\n";

	// Validate total length doesn't exceed 512 characters
	validateLength(result);

	return result;
}
