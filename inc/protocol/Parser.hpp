#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <iostream>
#include <stdexcept>
#include "MessageBuilder.hpp"

/**
 * @brief IRC Protocol Parser
 * 
 * This class is responsible for parsing raw IRC messages into structured format
 * According to RFC 1459 specifications
 */
class Parser {
	private:
			/**
			 * @brief Remove \r\n from the end of the string
			 * 
			 * @param str String to process
			 * @return String without \r\n at the end
			 */
			static std::string stripCRLF(const std::string& str);

			/**
			 * @brief Extract prefix from the message
			 * 
			 * @param line Current line being processed (will be modified)
			 * @return Extracted prefix or empty string if no prefix
			 * 
			 * @details
			 * If line starts with ':', extracts everything until first space
			 * and removes it from the line
			 */
			static std::string extractPrefix(std::string& line);

			/**
			 * @brief Extract command from the message
			 * 
			 * @param line Current line being processed (will be modified)
			 * @return Extracted command
			 * @throws std::invalid_argument if no command found
			 */
			static std::string	extractCommand(std::string& line);
			
			/**
			 * @brief Extract regular parameters and trailing parameter from the remaining line
			 * 
			 * @param line Input string after prefix and command extraction
			 * @param[out] params Vector to store regular space-separated parameters
			 * @param[out] trailing String to store trailing parameter (after ':')
			 */
			static void			extractParams(
				const std::string& line,
				std::vector<std::string>& params,
				std::string& trailing);

	public:
			Parser() = delete;
			~Parser() = delete;
			Parser(const Parser&) = delete;
			Parser& operator=(const Parser&) = delete;

			/**
			 * @brief Parse a raw IRC message into Message structure
			 * 
			 * @param raw The raw IRC message string (should end with \r\n)
			 * @return Parsed Message structure
			 * @throws std::invalid_argument if the message format is invalid
			 * 
			 * @details
			 * The function performs the following steps:
			 * 1. Remove trailing \r\n
			 * 2. Extract prefix if present (starts with ':')
			 * 3. Extract command (required)
			 * 4. Extract parameters and trailing
			 */
			static Message parse(const std::string& raw);

};

#endif
