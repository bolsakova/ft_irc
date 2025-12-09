#include "../inc/Parser.hpp"

/**
 * @brief Remove \r\n from the end of string
 * 
 * IRC protocol requires each message to end with \r\n
 * We need to remove it before parsing
 */
std::string Parser::stripCRLF(const std::string& str) {
	// Check if string is long enough to have \r\n
	if (str.length() < 2)
		return str;
	
	// Check if it actually ends with \r\n
	if (str[str.length() - 2] == '\r' && str[str.length() - 1] == '\n') {
		// Return string without last 2 characters
		return str.substr(0, str.length() - 2);
	}

	// If no \r\n found, return original string
	return str;
}

/**
 * @brief Extract prefix from IRC message
 * 
 * Prefix format: :servername or :nick[!user[@host]]
 * Must e at the beginning of the message
 */
std::string Parser::extractPrefix(std::string& line) {
	// Prefix must start with ':'
	if (line.empty() || line[0] != ':')
		return ""; // No prefix
	
	// Find the end of prefix (first space)
	size_t spacePos = line.find(' ');

	// If no space found, the entire line is prefix (invalid but we handle it)
	if (spacePos == std::string::npos) {
		std::string prefix = line.substr(1); // Skip the ':'
		line.clear(); // Nothing left in line
		return prefix;
	}

	// Extract prefix (without ':')
	std::string prefix = line.substr(1, spacePos - 1);

	// Remove prefix from line (including the space)
	line = line.substr(spacePos + 1);

	return prefix;
}

/**
 * @brief Extract command from IRC message
 * 
 * The command is the first word after prefix (if any)
 * Command ends with space or end of line
 * 
 * @param line Current line being processed (will be modified)
 * @return Extracted command
 * @throws std::invalid_argument if no command found
 */
std::string Parser::extractCommand(std::string& line) {
	// Step 1: Check if line is empty (no command = error)
	if (line.empty())
		throw std::invalid_argument("IRC message must have a command");
	
	// Step 2: Find where command ends (first apce or end of line)
	size_t spacePos = line.find(' ');

	// Step 3: Extract command
	std::string command;
	if (spacePos == std::string::npos) {
		// No space found = command is the entire remaining line
		command = line;
		line.clear();
	} else {
		// Space found = command is everything before the space
		command = line.substr(0, spacePos);
		line = line.substr(spacePos + 1); // Remove command and space from line
	}

	// Step 4: Validate command is not empty (extra safety)
	if (command.empty())
		throw std::invalid_argument("Command cannot be empty");

	return command;
}

/**
 * @brief Extract parameters and trailing from the remaining line
 * 
 * @param line Remaining line after command extraction
 * @param[out] params Vector to store regular parameters
 * @param[out] trailing String to store regular parameters
 */
void Parser::extractParams(const std::string& line, std::vector<std::string>& params, std::string& trailing) {
	// Clean output parameters
	params.clear();
	trailing.clear();

	// If line is empty, no parameters
	if (line.empty())
		return;

	// Working with a copy so we don't modify the original
	std::string remaining = line;

	// Process the line word by word
	while (!remaining.empty()) {
		// Check if we hit trailing parameter (starts with ':')
		if (remaining[0] == ':') {
			// Everything after ':' is trailing
			trailing = remaining.substr(1); // Skip the ':'
			break;
		}

		// Find next space (end of current parameter)
		size_t spacePos = remaining.find(' ');
		if (spacePos == std::string::npos) {
			// No more spaces = this is the last parameter
			params.push_back(remaining);
			break; // We're done
		}

		// Extract parameter (everything before space)
		std::string param = remaining.substr(0, spacePos);

		// Add to params vector
		params.push_back(param);

		// Remove processed parameter from remaining line
		remaining = remaining.substr(spacePos + 1);
	}

	/**
	 * @brief Parse a complete IRC message
	 * 
	 * This is the main parsing function that coordinates all steps
	 * 
	 * @param raw Raw IRC message (should end with \r\n)
	 * @return Parsed Message structure
	 * @throws std::invalid_argument if message format is invalid
	 */
	Message Parser::parse(const std::string& raw) {
		// Create empty message structure
		Message	msg;

		// Step 1: Remove \r\n from the end
		std::string line = stripCRLF(raw);

		// Step 2: Extract prefix if present
		msg 
	}
	
}
