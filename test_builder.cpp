#include "inc/protocol/Message.hpp"
#include <iostream>
#include <cassert>

/**
 * Test helper function to compare expected and actual output
 */
void testBuilder(const std::string& testName, const std::string& expected, const std::string& actual) {
	std::cout << "\n" << "--- Testing: " << testName << "\n";
	std::cout << "Expected: " << expected;
	std::cout << "Got: " << actual;

	if (expected == actual)
		std::cout << "✓ PASS" << "\n";
	else {
		std::cout << "✗ FAIL" << "\n";
		std::cout << "Mismatch detected!" << "\n";
	}
}

/**
 * Test numeric reply messages
 */
void testNumericReplies() {
	std::cout << "\n" << "=== Testing Numeric Replies" << "\n";

	// Test 1: Welcome message (001)
	{
		std::string result = MessageBuilder::buildNumericReply(
			"ircserv", 1, "tanja", "Welcome to the IRC Network"
		);
		testBuilder(
			"RPL_WELCOME (001)",
			":ircserv 001 tanja :Welcome to the IRC Network\r\n",
			result
		);
	}

	// Test 2: Your host message (002)
	{
		std::string result = MessageBuilder::buildNumericReply(
			"ircserv", 2, "tanja", "Your host is ircserv, running version 1.0"
		);
		testBuilder(
			"RPL_YOURHOST (002)",
			":ircserv 002 tanja :Your host is ircserv, running version 1.0\r\n",
			result
		);
	}
	// Test 3: Code formatting (099)
	{
		std::string result = MessageBuilder::buildNumericReply(
			"ircserv", 99, "tanja", "Test message"
		);
		testBuilder(
			"Code formatting (99 -> 099)",
			":ircserv 099 tanja :Test message\r\n",
			result
		);
	}
	// Test 4: Unknown target (*)
	{
		std::string result = MessageBuilder::buildNumericReply(
			"ircserv", 1, "*", "Welcome"
		);
		testBuilder(
			"Unknown target (*)",
			":ircserv 001 * :Welcome\r\n",
			result
		);
	}
}

/**
 * Test error messages
 */
void testErrorMessages() {
	std::cout << "\n" << "=== Testing Error Messages ===" << "\n";

	// Test 1: Nickname in use (433)
	{
		std::string result = MessageBuilder::buildError(
			"ircserv", 433, "*", "tanja", "Nickname is already in use"
		);
		testBuilder(
			"ERR_NICKNAMEINUSE (433)",
			":ircserv 433 * tanja :Nickname is already in use\r\n",
			result
		);
	}

	// Test 2: No such nick (401)
	{
		std::string result = MessageBuilder::buildError(
			"ircserv", 401, "alice", "bob", "No such nick/channel"
		);
		testBuilder(
			"ERR_NOSUCHNICK (401)",
			":ircserv 401 alice bob :No such nick/channel\r\n",
			result
		);
	}

	// Test 3: No such channel (403)
	{
		std::string result = MessageBuilder::buildError(
			"ircserv", 403, "alice", "#test", "No such channel"
		);
		testBuilder(
			"ERR_NOSUCHCHANNEL (403)",
			":ircserv 403 alice #test :No such channel\r\n",
			result
		);
	}

	// Test 4: Not enough parameters (461)
	{
		std::string result = MessageBuilder::buildError(
			"ircserv", 461, "alice", "JOIN", "Not enough parameters"
		);
		testBuilder(
			"ERR_NEEDMOREPARAMS (461)",
			":ircserv 461 alice JOIN :Not enough parameters\r\n",
			result
		);
	}
}

/**
 * Test command messages
 */
void testCommandMessages() {
	std::cout << "\n" << "=== Testing Command messages ===" << "\n";

	// Test 1: PRIVMSG with trailing
    {
        std::string result = MessageBuilder::buildCommand(
            "alice!user@host", "PRIVMSG", {"bob"}, "Hello there!"
        );
        testBuilder(
            "PRIVMSG with trailing",
            ":alice!user@host PRIVMSG bob :Hello there!\r\n",
            result
        );
    }
    
    // Test 2: JOIN without trailing
    {
        std::string result = MessageBuilder::buildCommand(
            "alice!user@host", "JOIN", {"#channel"}
        );
        testBuilder(
            "JOIN without trailing",
            ":alice!user@host JOIN #channel\r\n",
            result
        );
    }

	// Test 3: PART with trailing
    {
        std::string result = MessageBuilder::buildCommand(
            "alice!user@host", "PART", {"#channel"}, "Goodbye!"
        );
        testBuilder(
            "PART with trailing",
            ":alice!user@host PART #channel :Goodbye!\r\n",
            result
        );
    }
    
    // Test 4: MODE with multiple params
    {
        std::string result = MessageBuilder::buildCommand(
            "alice!user@host", "MODE", {"#channel", "+o", "bob"}
        );
        testBuilder(
            "MODE with multiple params",
            ":alice!user@host MODE #channel +o bob\r\n",
            result
        );
    }

	// Test 5: KICK with trailing
    {
        std::string result = MessageBuilder::buildCommand(
            "alice!user@host", "KICK", {"#channel", "bob"}, "Bad behavior"
        );
        testBuilder(
            "KICK with trailing",
            ":alice!user@host KICK #channel bob :Bad behavior\r\n",
            result
        );
    }
    
    // Test 6: QUIT with trailing (empty params)
    {
        std::vector<std::string> emptyParams;
        std::string result = MessageBuilder::buildCommand(
            "alice!user@host", "QUIT", emptyParams, "Leaving IRC"
        );
        testBuilder(
            "QUIT with empty params",
            ":alice!user@host QUIT :Leaving IRC\r\n",
            result
        );
    }
}

/**
 * Test message length validation
 */
void testLengthValidation() {
	std::cout << "\n" << "=== Testing Length Validation ===" << "\n";
    
    // Test: Message that exceeds 512 characters
    try {
        // Create a very long message (> 512 characters)
        std::string longMessage(500, 'A');
        std::string result = MessageBuilder::buildNumericReply(
            "ircserv", 1, "tanja", longMessage
        );
        std::cout << "✗ FAIL: Should have thrown exception for long message" << "\n";
    } catch (const std::length_error& e) {
        std::cout << "✓ PASS: Correctly threw exception: " << e.what() << "\n";
    }
}

int main() {
	std::cout << "=== IRC Message Builder Test Suite ===" << "\n";
	std::cout << "\n";

	testNumericReplies();
	testErrorMessages();
	testCommandMessages();
	testLengthValidation();

	std::cout << "\n" << "=== All Tests Complete ===" << "\n";

	return 0;
}
