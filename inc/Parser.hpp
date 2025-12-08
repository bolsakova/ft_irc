#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>
#include <vector>
#include <iostream>
#include "Message.hpp"

class Parser {
	private:

	public:
		std::string	extractCommand(std::string& line);
		void		extractParams(const std::string& line, std::vector<std::string>& params, std::string& trailing);
		Message		parse(const std::string& raw);
};

#endif
