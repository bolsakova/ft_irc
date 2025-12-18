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
