#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

/**
 * @brief Structure representing a parsed IRC message
 * 
 * IRC message format: [:<prefix>] <command> [<params>] [:<trailing>]
 * Example: :tanja!user@host PRIVMSG #channel :Hello everyone!
 */
struct Message {
	/**
	 * @brief Optional prefix indicating the source of the message
	 * Format can be: servername or nick[!user[@host]]
	 * Example: "tanja!user@host" or "irc.example.com"
	 */
	std::string prefix;

	/**
	 * @brief The IRC command (always required)
	 * Can be either a word (PRIVMSG) or numeric code (001)
	 */
	std::string command;

	/**
	 * @brief List of parameters (excluding trailing)
	 * Each parameter is a separate string without spaces
	 * Example: for "MODE #channel +o alice", params = ["#channel", "+o", "tanja"]
	 */
	std::vector<std::string> params;

	/**
	 * @brief The trailing parameter (optional)
	 * Can contain spaces and special characters
	 * Everything after " :" until the end of line
	 */
	std::string trailing;

	/**
	 * @brief Check if the message has a prefix
	 * @return True - if prefix is not empty, False - otherwise
	 */
	bool hasPrefix() const {
		return !prefix.empty();
	}
	
	/**
	 * @brief Check if the message has a trailing parameter
	 * @return True - if trailing is not empty, False - otherwise
	 */
	bool hasTrailing() const {
	   return !trailing.empty();
	}

	/**
	 * @brief Get the total number of parameters (including trailing if present)
	 * @return Number of parameters
	 */
	size_t getTotalParams() const {
		return params.size() + (hasTrailing() ? 1 : 0);
	}
};

#endif
