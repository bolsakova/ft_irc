#include "inc/protocol/Parser.hpp"
#include "inc/protocol/Message.hpp"

void testParse(const std::string& raw) {
	std::cout << "\n--- Testing: " << raw;

	try {
		Message msg = Parser::parse(raw);

		std::cout << "Prefix: [" << msg.prefix << "]\n";
		std::cout << "Command: [" << msg.command << "]\n";
		std::cout << "Params: (" << msg.params.size() << "): ";
		for (const auto& p : msg.params)
			std::cout << "[" << p << "] ";
		std::cout << "\nTrailing: [" << msg.trailing << "]\n";

	} catch (const std::exception& e) {
		std::cout << "Error: " << e.what() << "\n";
	}
}

int main() {
	// Test different message formats
	testParse("NICK tanja\r\n");
	testParse("USER tanja 0 * :Tanja tbolsako\r\n");
	testParse(":server 001 tanja :Welcome!\r\n");
	testParse("PRIVMSG #channel :Hello everyone!\r\n");
	testParse("MODE #channel +o tanja\r\n");
}
