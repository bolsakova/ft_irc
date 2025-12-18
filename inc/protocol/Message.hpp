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

/**
 * @brief IRC Message Builder
 * 
 * This class is  responsible for constructing IRC protocol messages
 * to be sent from server to clients according to RFC 1459 specifications
 */
class MessageBuilder {
	private:
			/**
			 * @brief Format numeric code to 3-digit string (1 -> 001)
			 * @param code The numeric code (1-999)
			 * @return Three-digit string representation 
			 */
			static std::string formatCode(int code);

			/**
			 * @brief Validate message length (max 512 characters)
			 * @param message The complete message to validate
			 * @throws std::length_error if message exceeds 512 characters
			 */
			static void validateLength(const std::string& message);

	public:
			/**
			 * @brief Build numeric reply message (001,002,003...)
			 * @param server Server name sending the reply
			 * @param code Numeric reply code
			 * @param target Target nickname
			 * @param message Message text
			 * @return Formatted message: ":<server> <code> <target> :<message>\r\n"
			 */
			static std::string buildNumericReply(
				const std::string& server,
				int code,
				const std::string& target,
				const std::string& message
			);
			
			/**
			 * @brief Build error reply message (4xx,5xx)
			 * @param server Server name sending the error
			 * @param code Error code
			 * @param target Target nickname (or "*" if unknown)
			 * @param param Additional parameter (problematic nick/channel)
			 * @param message Error message text
			 * @return Formatted message: ":<server> <code> <target> <param> :<message>\r\n"
			 */
			static std::string buildError(
				const std::string& server,
				int code,
				const std::string& target,
				const std::string& param,
				const std::string& message
			);

			/**
			 * @brief Build command message from server
			 * @param prefix Message source (nick!user@host or servername)
			 * @param command IRC command (JOIN, PART, PRIVMSG, etc.)
			 * @param params Vector of regular parameters
			 * @param trailing Optional trailing parameter (can contain spaces)
			 * @return Formatted message: ":<prefix> <command> [params] [:<trailing>]\r\n"
			 */
			static std::string buildCommand(
				const std::string& prefix,
				const std::string& command,
				const std::vector<std::string>& params,
				const std::string& trailing = ""
			);
};

#endif
