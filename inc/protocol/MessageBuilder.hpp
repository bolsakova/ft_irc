#ifndef MESSAGEBUILDER_HPP
#define MESSAGEBUILDER_HPP

#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <stdexcept>

/**
 * @file MessageBuilder.hpp
 * @brief IRC message structures and builder utilities
 * 
 * IRC message format: [:<prefix>] <command> [<params>] [:<trailing>]
 * Example: :tanja!user@host PRIVMSG #channel :Hello everyone!
 * 
 * Provides Message structure for parsed IRC messages and MessageBuilder
 * for constructing protocol-compliant responses according to RFC 1459.
 */

// Parsed IRC message structure
struct Message {
	std::string prefix;					// Optional prefix indicating the source of the message
	std::string command;				// IRC command (always required)
	std::vector<std::string> params;	// List of parameters (excluding trailing)
	std::string trailing;				// trailing parameter (optional)

	bool hasPrefix() const;
	bool hasTrailing() const;
	size_t getTotalParams() const;
};

// IRC message builder for server responses
class MessageBuilder {
	private:
			static std::string	formatCode(int code);				// Format numeric code to 3-digit string (1 -> 001)
			static void			validateLength(const std::string& message);	// Validate message length (max 512 characters)

	public:
			MessageBuilder() = delete;
			~MessageBuilder() = delete;
			MessageBuilder(const MessageBuilder&) = delete;
			MessageBuilder&		operator=(const MessageBuilder&) = delete;
	
			// Build numeric reply (001, 002, 003...)
			static std::string	buildNumericReply(const std::string& server, int code, const std::string& target, const std::string& message);
			
			// Build error reply (4xx,5xx)
			static std::string	buildErrorReply(const std::string& server, int code, const std::string& target, const std::string& param, const std::string& message);

			// Build command message from server (JOIN, PART, PRIVMSG, etc.)
			static std::string	buildCommandMessage(const std::string& prefix, const std::string& command, const std::vector<std::string>& params, const std::string& trailing = "");
};

#endif
