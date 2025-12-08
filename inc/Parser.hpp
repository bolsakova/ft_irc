#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <iostream>
#include <stdexcept>
#include "Message.hpp"

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


			std::string	extractCommand(std::string& line);
			void		extractParams(const std::string& line, std::vector<std::string>& params, std::string& trailing);

	public:
			/**
			 * @brief Parse a raw IRC message into Message structure
			 * 
			 * @param raw The raw IRC message string (should end with \r\n)
			 * @return Parsed Message structure
			 * @throws std::invalid_argument if the message format is invalid
			 * 
			 * @details
			 * The function performs the followig steps:
			 * 1. Remove trailing \r\n
			 * 2. Extract prefix if present (starts with ':')
			 * 3. Extrat command (required)
			 * 4. Extract parameters and trailing
			 */

			static Message parse(const std::string& raw);

};

#endif
