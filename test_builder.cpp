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

}

/**
 * Test error messages
 */
void testErrorMessages() {
	
}

/**
 * Test command messages
 */
void testCommandMessages() {

}

/**
 * Test message length validation
 */
void testLengthValidation() {

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
