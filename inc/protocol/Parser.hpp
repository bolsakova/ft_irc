#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <iostream>
#include <stdexcept>
#include "MessageBuilder.hpp"

/**
 * @brief IRC protocol message Parser
 * 
 * Parses raw IRC messages into structured Message format according to
 * RFC 1459 specifications. Handles prefix extraction, command parsing,
 * and parameter separation.
 */
class Parser {
	private:
			static std::string	stripCRLF(const std::string& str);																	// Remove \r\n from the end of the string
			static std::string	extractPrefix(std::string& line);																	// Extract prefix from the message
			static std::string	extractCommand(std::string& line);																	// Extract command from the message
			static void			extractParams(const std::string& line, std::vector<std::string>& paarms, std::string& trailing);	// Extract regular parameters and trailing parameter from the remaining line

	public:
			Parser() = delete;
			~Parser() = delete;
			Parser(const Parser&) = delete;
			Parser&				operator=(const Parser&) = delete;

			// Parse a raw IRC message into Message structure
			static Message		parse(const std::string& raw);

};

#endif
