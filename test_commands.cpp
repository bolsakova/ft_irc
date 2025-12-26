#include "inc/protocol/CommandHandler.hpp"
#include "inc/protocol/Parser.hpp"
#include "inc/protocol/MessageBuilder.hpp"
#include "inc/protocol/Replies.hpp"
#include "inc/network/Server.hpp"
#include "inc/network/Client.hpp"
#include <cassert> //

// class MockServer {
// 	private:
// 			std::map<int, std::unique_ptr<Client>> m_clients;
// 			std::map<std::string, std::unique_ptr<Channel>> m_channels;
// 			std::string m_password;
	
// 	public:
// 			MockServer(const std::string& password) : m_password(password) {}
    
// 			const std::map<int, std::unique_ptr<Client>>& getClients() const {
// 				return m_clients;
// 			}
			
// 			const std::map<std::string, std::unique_ptr<Channel>>& getChannels() const {
// 				return m_channels;
// 			}
			
// 			Client* addClient(int fd) {
// 				m_clients[fd] = std::make_unique<Client>(fd);
// 				return m_clients[fd].get();
// 			}

// 			Channel* findChannel(const std::string& name) {
// 				auto it = m_channels.find(name);
// 				return (it != m_channels.end()) ? it->second.get() : nullptr;
// 			}
    
// 			Channel* createChannel(const std::string& name) {
// 				if (!findChannel(name)) {
// 					m_channels[name] = std::make_unique<Channel>();
// 					// Set channel name here if Channel has setName method
// 				}
// 				return m_channels[name].get();
// 			}
			
// 			void removeChannel(const std::string& name) {
// 				m_channels.erase(name);
// 			}

// 			const std::string& getPassword() const {
// 				return m_password;
// 			}
// };

void printTestResult(const std::string& testName, bool passed) {
	std::cout << "  " << testName << ": ";
	if (passed)
		std::cout << "✓ PASS\n";
	else
		std::cout << "✗ FAIL\n";
}

void testPingCommand([[maybe_unused]] Server& server, CommandHandler& handler) {
	std::cout << "\n=== Testing PING Command ===\n";

	// Create test client
	Client testClient(100);

	// Test 1: PING without registration should fail
	std::cout << "\nTest 1: PING without registration\n";
	testClient.setAuthenticated(false);
	testClient.setRegistered(false);

	handler.handleCommand("PING :test\r\n", testClient);
	std::string response = testClient.getOutBuf();

	bool hasError = (response.find("451") != std::string::npos);
	std::cout << " Response: " << response;
	printTestResult("ERR_NOTREGISTERED", hasError);

	// Test 2: PING with registration should work
	std::cout << "\nTest 2: PING with registration\n";
	testClient.setAuthenticated(true);
	testClient.setNickname("tanja");
	testClient.setUsername("tanja");
	testClient.setRealname("Tanja B");
	testClient.setRegistered(true);

	// Clear buffer
	testClient.consumeOutBuf(testClient.getOutBuf().size());

	handler.handleCommand("PING :test123\r\n", testClient);
	response = testClient.getOutBuf();

	bool hasPong = (response.find("PONG") != std::string::npos);
	bool hasToken = (response.find("test123") != std::string::npos);

	std::cout << " Response: " << response;
	printTestResult("PONG with token", hasPong && hasToken);

	// Test 3: PING without token
	std::cout << "\nTest 3: PING without token (default)\n";
	testClient.consumeOutBuf(testClient.getOutBuf().size());

	handler.handleCommand("PING\r\n", testClient);
	response = testClient.getOutBuf();

	hasPong = (response.find("PONG") != std::string::npos);

	std::cout << " Response: " << response;
	printTestResult("PONG with default token", hasPong);
}

void testQuitCommand([[maybe_unused]] Server& server, CommandHandler& handler) {
	std::cout << "\n=== Testing QUIT Command ===\n";

	// Test 1: QUIT with reason
	std::cout << "\nTest 1: QUIT with reason\n";
	Client testClient(101);
	testClient.setAuthenticated(true);
	testClient.setNickname("alice");
	testClient.setUsername("alice");
	testClient.setRegistered(true);

	handler.handleCommand("QUIT :Going to sleep\r\n", testClient);

	bool marked = testClient.shouldDisconnect();
	std::string reason = testClient.getQuitReason();

	std::cout << " Marked for disconnect: " << (marked ? "yes" : "no") << "\n";
	std::cout << " Quit reason: [" << reason << "]\n";
	
	printTestResult("QUIT with custom reason", marked && reason == "Going to sleep");

	// Test 2: QUIT without reason
	std::cout << "\nTest 2: QUIT without reason\n";
	Client testClient2(102);
	testClient2.setAuthenticated(true);
	testClient2.setNickname("bob");
	testClient2.setUsername("bob");
	testClient2.setRegistered(true);

	handler.handleCommand("QUIT\r\n", testClient2);

	marked = testClient2.shouldDisconnect();
	reason = testClient2.getQuitReason();

	std::cout << " Quit reason: [" << reason << "]\n";
	
	printTestResult("QUIT with default reason", marked && reason == "Client exited");

	// Test 3: QUIT even without registration (should work)
	std::cout << "\nTest 3: QUIT even without registration\n";
	Client testClient3(103);
	testClient3.setRegistered(false);

	handler.handleCommand("QUIT :Bye\r\n", testClient3);

	marked = testClient3.shouldDisconnect();
	
	printTestResult("QUIT without registration", marked);
}

void testPrivmsgCommand(Server& server, CommandHandler& handler) {
	std::cout << "\n=== Testing PRIVMSG Command ===\n";

	// Test 1: PRIVMSG without registration
	std::cout << "\nTest 1: PRIVMSG without registration\n";
	Client unregistered(104);
	unregistered.setRegistered(false);

	handler.handleCommand("PRIVMSG alice :Hello\r\n", unregistered);
	std::string response = unregistered.getOutBuf();

	bool hasError = (response.find("451") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTREGISTERED", hasError);

	// Test 2: PRIVMSG without recipient
	std::cout << "\nTest 2: PRIVMSG without recipient\n";
	Client sender(105);
	sender.setAuthenticated(true);
	sender.setNickname("tanja");
	sender.setUsername("tanja");
	sender.setRegistered(true);

	handler.handleCommand("PRIVMSG\r\n", sender);
	response = sender.getOutBuf();

	hasError = (response.find("411") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NORECIPIENT", hasError);

	// Test 3: PRIVMSG without text
	std::cout << "\nTest 3: PRIVMSG without text\n";
	sender.consumeOutBuf(sender.getOutBuf().size());

	handler.handleCommand("PRIVMSG alice\r\n", sender);
	response = sender.getOutBuf();

	hasError = (response.find("412") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTEXTTOSEND", hasError);

	// Test 4: PRIVMSG to non-existent user
	std::cout << "\nTest 4: PRIVMSG to non-existent user\n";
	sender.consumeOutBuf(sender.getOutBuf().size());

	handler.handleCommand("PRIVMSG nonexistent :Hello\r\n", sender);
	response = sender.getOutBuf();

	hasError = (response.find("401") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHNICK", hasError);

	// Test 5: PRIVMSG to non-existent channel (should return ERR_NOSUCHCHANNEL)
	std::cout << "\nTest 5: PRIVMSG to non-existent channel\n";
	sender.consumeOutBuf(sender.getOutBuf().size());

	handler.handleCommand("PRIVMSG #general :Hello channel\r\n", sender);
	response = sender.getOutBuf();

	hasError = (response.find("403") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHCHANNEL", hasError);

	// Test 6: PRIVMSG to existing channel with membership
	std::cout << "\nTest 6: PRIVMSG to existing channel\n";

	Channel* testChan = server.createChannel("#testChan");
	testChan->addMember(&sender);

	Client receiver(106);
	receiver.setAuthenticated(true);
	receiver.setNickname("alice");
	receiver.setUsername("alice");
	receiver.setRegistered(true);
	testChan->addMember(&receiver);

	sender.consumeOutBuf(sender.getOutBuf().size());
	receiver.consumeOutBuf(receiver.getOutBuf().size());

	handler.handleCommand("PRIVMSG #testChan :Hello everyone!\r\n", sender);
	
	std::string receiverBuf = receiver.getOutBuf();

	bool hasMessage = (receiverBuf.find("Hello everyone!") != std::string::npos);
	bool hasPrivmsg = (receiverBuf.find("PRIVMSG") != std::string::npos);

	std::cout << "  Receiver got: " << receiverBuf;
	printTestResult("Channel PRIVMSG delivery", hasMessage && hasPrivmsg);

	std::string senderBuf = sender.getOutBuf();
	bool senderEmpty = senderBuf.empty();

	printTestResult("Sender didn't receive own message", senderEmpty);
}

void testJoinCommand(Server& server, CommandHandler& handler) {
	std::cout << "\n=== Testing JOIN Command ===\n";

	// Test 1: JOIN without registration
	std::cout << "\nTest 1: JOIN without registration\n";
	Client unregistered(300);
	unregistered.setRegistered(false);

	handler.handleCommand("JOIN #test\r\n", unregistered);
	std::string response = unregistered.getOutBuf();

	bool hasError = (response.find("451") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTREGISTERED", hasError);

	// Test 2: JOIN without channel parameter
	std::cout << "\nTest 2: JOIN without channel parameter\n";
	Client registered(301);
	registered.setAuthenticated(true);
	registered.setNickname("alice");
	registered.setUsername("alice");
	registered.setRegistered(true);

	handler.handleCommand("JOIN\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("461") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NEEDMOREPARAMS", hasError);

	// Test 3: JOIN with invalid channel name
	std::cout << "\nTest 3: JOIN with invalid channel name\n";
	registered.consumeOutBuf(registered.getOutBuf().size());

	handler.handleCommand("JOIN invalidname\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("403") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHCHANNEL", hasError);

	// Test 4: JOIN create new channel (becomes operator)
	std::cout << "\nTest 4: JOIN create new channel (becomes operator)\n";
	Client creator(302);
	creator.setAuthenticated(true);
	creator.setNickname("creator");
	creator.setUsername("creator");
	creator.setRegistered(true);

	handler.handleCommand("JOIN #newchan\r\n", creator);
	response = creator.getOutBuf();

	bool hasJoin = (response.find("JOIN") != std::string::npos);
	bool hasNames = (response.find("353") != std::string::npos);			// RPL_NAMREPLY
	bool hasEndNames = (response.find("366") != std::string::npos);			// RPL_ENDOFNAMES
	bool hasOperator = (response.find("@creator") != std::string::npos);	// @ prefix

	std::cout << "  Has JOIN: " << (hasJoin ? "yes" : "no") << "\n";
	std::cout << "  Has NAMES: " << (hasNames ? "yes" : "no") << "\n";
	std::cout << "  Is operator: " << (hasOperator ? "yes" : "no") << "\n";

	printTestResult("Create channel as operator", hasJoin && hasNames && hasEndNames && hasOperator);

	// Test 5: JOIN existing channel (not operator)
	std::cout << "\nTest 5: JOIN existing channel\n";
	Client joiner(303);
	joiner.setAuthenticated(true);
	joiner.setNickname("joiner");
	joiner.setUsername("joiner");
	joiner.setRegistered(true);

	handler.handleCommand("JOIN #newchan\r\n", joiner);
	response = joiner.getOutBuf();

	hasJoin = (response.find("JOIN") != std::string::npos);
	bool notOperator = (response.find("@joiner") == std::string::npos);	// No @ prefix
	bool hasBothUsers = (response.find("creator") != std::string::npos &&
						response.find("joiner") != std::string::npos);

	std::cout << "  Has JOIN: " << (hasJoin ? "yes" : "no") << "\n";
	std::cout << "  Not operator: " << (notOperator ? "yes" : "no") << "\n";
	std::cout << "  Both users in list: " << (hasBothUsers ? "yes" : "no") << "\n";

	printTestResult("Join existing channel", hasJoin && notOperator && hasBothUsers);

	// Test 6: JOIN with invite-only mode (+i)
	std::cout << "\nTest 6: JOIN invite-only channel without invite\n";

	// Create invite-only channel
	Channel* inviteChan = server.createChannel("#invite");
	inviteChan->setInviteOnly(true);

	Client noInvite(304);
	noInvite.setAuthenticated(true);
	noInvite.setNickname("noinvite");
	noInvite.setUsername("noinvite");
	noInvite.setRegistered(true);

	handler.handleCommand("JOIN #invite\r\n", noInvite);
	response = noInvite.getOutBuf();

	hasError = (response.find("473") != std::string::npos);	// ERR_INVITEONLYCHAN
	std::cout << "  Response: " << response;
	printTestResult("ERR_INVITEONLYCHAN", hasError);

	// Test 7: JOIN with key mode (+k) - wrong key
	std::cout << "\nTest 7: JOIN channel with key - wrong key\n";

	Channel* keyChan = server.createChannel("#secret");
	keyChan->setKey("correctkey");

	Client wrongKey(305);
	wrongKey.setAuthenticated(true);
	wrongKey.setNickname("wrongKey");
	wrongKey.setUsername("wrongKey");
	wrongKey.setRegistered(true);

	handler.handleCommand("JOIN #secret wrongkey\r\n", wrongKey);
	response = wrongKey.getOutBuf();

	hasError = (response.find("475") != std::string::npos);	// ERR_BADCHANNELKEY
	std::cout << "  Response: " << response;
	printTestResult("ERR_BADCHANNELKEY", hasError);

	// Test 8: JOIN with correct key
	std::cout << "\nTest 8: JOIN with correct key\n";
	wrongKey.consumeOutBuf(wrongKey.getOutBuf().size());

	handler.handleCommand("JOIN #secret correctkey\r\n", wrongKey);
	response = wrongKey.getOutBuf();

	hasJoin = (response.find("JOIN") != std::string::npos);
	std::cout << "  Has JOIN: " << (hasJoin ? "yes" : "no") << "\n";
	printTestResult("JOIN with correct key", hasJoin);

	// Test 9: JOIN with user limit (+l) - channel full
	std::cout << "\nTest 9: JOIN channel with user limit (full)\n";

	Channel* limitChan = server.createChannel("#limited");
	limitChan->setUserLimit(1);

	Client first(306);
	first.setAuthenticated(true);
	first.setNickname("first");
	first.setUsername("first");
	first.setRegistered(true);
	limitChan->addMember(&first);

	Client second(307);
	second.setAuthenticated(true);
	second.setNickname("second");
	second.setUsername("second");
	second.setRegistered(true);

	handler.handleCommand("JOIN #limited\r\n", second);
	response = second.getOutBuf();

	hasError = (response.find("471") != std::string::npos);	// ERR_CHANNELISFULL
	std::cout << "  Response: " << response;
	printTestResult("ERR_CHANNELISFULL", hasError);
}

void testPartCommand(Server& server, CommandHandler& handler)
{
	std::cout << "\n=== Testing PART Command ===\n";

	// Test 1: PART without registration
	std::cout << "\nTest 1: PART without registration\n";
	Client unregistered(400);
	unregistered.setRegistered(false);

	handler.handleCommand("PART #test\r\n", unregistered);
	std::string response = unregistered.getOutBuf();

	bool hasError = (response.find("451") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTREGISTERED", hasError);

	// Test 2: PART without channel parameter
	std::cout << "\nTest 2: PART without channel parameter\n";
	Client registered(401);
	registered.setAuthenticated(true);
	registered.setNickname("alice");
	registered.setUsername("alice");
	registered.setRegistered(true);

	handler.handleCommand("PART\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("461") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NEEDMOREPARAMS", hasError);

	// Test 3: PART from non-existent channel
	std::cout << "\nTest 3: PART from non-existent channel\n";
	Client client(402);
	client.setAuthenticated(true);
	client.setNickname("testuser");
	client.setUsername("testuser");
	client.setRegistered(true);

	handler.handleCommand("PART #nonexistent\r\n", client);
	response = client.getOutBuf();

	hasError = (response.find("403") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHCHANNEL", hasError);

	// Test 4: PART when not on channel
	std::cout << "\nTest 4: PART when not on channel\n";

	// Create channel with another user
	Client creator(403);
	creator.setAuthenticated(true);
	creator.setNickname("creator");
	creator.setUsername("creator");
	creator.setRegistered(true);

	handler.handleCommand("JOIN #testchan\r\n", creator);
	creator.consumeOutBuf(creator.getOutBuf().size());
	
	// Try to PART from someone not on channel
	Client notMember(404);
	notMember.setAuthenticated(true);
	notMember.setNickname("notmember");
	notMember.setUsername("notmember");
	notMember.setRegistered(true);
	
	handler.handleCommand("PART #testchan\r\n", notMember);
	response = notMember.getOutBuf();

	hasError = (response.find("442") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTONCHANNEL", hasError);

	// Test 5: Successful PART without reason
	std::cout << "\nTest 5: Successful PART without reason\n";
	Client partUser(405);
	partUser.setAuthenticated(true);
	partUser.setNickname("partuser");
	partUser.setUsername("partuser");
	partUser.setRegistered(true);

	// Join channel first
	handler.handleCommand("JOIN #parttest\r\n", partUser);
	partUser.consumeOutBuf(partUser.getOutBuf().size());
	
	// Part from channel
	handler.handleCommand("PART #parttest\r\n", partUser);
	response = partUser.getOutBuf();

	bool hasPart = (response.find("PART") != std::string::npos);	// No @ prefix
	bool hasChannel = (response.find("#parttest") != std::string::npos);

	std::cout << "  Has PART: " << (hasPart ? "yes" : "no") << "\n";
	std::cout << "  Has channel: " << (hasChannel ? "yes" : "no") << "\n";
	printTestResult("PART without reason", hasPart && hasChannel);

	// Verify user is removed from channel
	Channel* chan = server.findChannel("#parttest");
	bool removed = (!chan || !chan->isMember(405));
	std::cout << " User removed: " << (removed ? "yes" : "no") << "\n";
	printTestResult("User removed from channel", removed);

	// Test 6: Successful PART with reason
	std::cout << "\nTest 6: Successful PART with reason\n";

	Client reasonUser(406);
	reasonUser.setAuthenticated(true);
	reasonUser.setNickname("reasonuser");
	reasonUser.setUsername("reasonuser");
	reasonUser.setRegistered(true);

	// Join channel
	handler.handleCommand("JOIN #reasontest\r\n", reasonUser);
	reasonUser.consumeOutBuf(reasonUser.getOutBuf().size());
	
	// Part with reason
	handler.handleCommand("PART #reasontest :Goodbye everyone!\r\n", reasonUser);
	response = reasonUser.getOutBuf();

	hasPart = (response.find("PART") != std::string::npos);
	bool hasReason = (response.find("Goodbye everyone!") != std::string::npos);

	std::cout << "  Has PART: " << (hasPart ? "yes" : "no") << "\n";
	std::cout << "  Has reason: " << (hasReason? "yes" : "no") << "\n";
	printTestResult("PART with reason", hasPart && hasReason);

	// Test 7: PART broadcasts to channel members
	std::cout << "\nTest 7: PART broadcasts to channel members\n";

	Client user1(407);
	user1.setAuthenticated(true);
	user1.setNickname("user1");
	user1.setUsername("user1");
	user1.setRegistered(true);

	Client user2(408);
	user2.setAuthenticated(true);
	user2.setNickname("user2");
	user2.setUsername("user2");
	user2.setRegistered(true);

	// Both join channel
	handler.handleCommand("JOIN #broadcast\r\n", user1);
	handler.handleCommand("JOIN #broadcast\r\n", user2);
	
	user1.consumeOutBuf(user1.getOutBuf().size());
	user2.consumeOutBuf(user2.getOutBuf().size());
	
	// User1 parts with message
	handler.handleCommand("PART #broadcast :Leaving now\r\n", user1);

	// User1 should see their own PART
	std::string output1 = user1.getOutBuf();
	bool user1SeesPart = (output1.find("PART") != std::string::npos);
	std::cout << "  User1 sees PART: " << (user1SeesPart ? "yes" : "no") << "\n";
	
	std::string output2 = user2.getOutBuf();
	bool user2SeesPart = (output2.find("PART") != std::string::npos);
	bool user2SeesUser1 = (output2.find("user1") != std::string::npos);
	bool user2SeesReason = (output2.find("Leaving now") != std::string::npos);
	
	std::cout << "  User2 sees PART: " << (user2SeesPart ? "yes" : "no") << "\n";
	std::cout << "  User2 sees user1: " << (user2SeesUser1 ? "yes" : "no") << "\n";
	std::cout << "  User2 sees reason: " << (user2SeesReason ? "yes" : "no") << "\n";

	printTestResult("PART broadcasts to members", user1SeesPart && user2SeesPart && user2SeesUser1 && user2SeesReason);
}

/**
 * @brief Test Registration Flow (PASS -> NICK -> USER)
 */
void testRegistrationFlow([[maybe_unused]] Server& server, CommandHandler& handler) {
	std::cout << "\n=== Testing Registration Flow ===\n";

	Client testClient(200);

	std::cout << "\nStep 1: PASS command\n";
	handler.handleCommand("PASS password123\r\n", testClient);
	bool authenticated = testClient.isAuthenticated();
	std::cout << " Authenticated: " << (authenticated ? "yes" : "no") << "\n";
	printTestResult("PASS authentication", authenticated);
	
	std::cout << "\nStep 2: NICK command\n";
	handler.handleCommand("NICK tanja\r\n", testClient);
	std::string nick = testClient.getNickname();
	std::cout << " Nickname: [" << nick << "]\n";
	printTestResult("NICK set", nick == "tanja");

	std::cout << "\nStep 2: USER command\n";
	testClient.consumeOutBuf(testClient.getOutBuf().size());
	handler.handleCommand("USER tanja 0 * :Tanja B\r\n", testClient);
	
	bool registered = testClient.isRegistered();
	std::string username = testClient.getUsername();
	std::string response = testClient.getOutBuf();
	
	std::cout << " Registered: " << (registered ? "yes" : "no") << "\n";
	std::cout << " Username: [" << username << "]\n";
	
	bool hasWelcome = (response.find("001") != std::string::npos);
	std::cout << " Welcome message: " << (hasWelcome ? "sent" : "not sent") << "\n";

	printTestResult("Complete registration",
		authenticated && registered && nick == "tanja" &&
		username == "tanja" && hasWelcome);
}

int main() {
	std::cout << "======================================\n";
	std::cout << "    IRC Command Handler Test Suite   \n";
	std::cout << "======================================\n";
	
	try {
		// Create server (won't run network loop, just for testing)
		std::cout << "\nInitializing test server...\n";
		Server server("6667", "password123");
		CommandHandler handler(server, "password123");
		
		std::cout << "Server initialized for testing.\n";

		// Run tests
		// testRegistrationFlow(server, handler);
		// testPingCommand(server, handler);
		// testQuitCommand(server, handler);
		// testPrivmsgCommand(server, handler);
		// testJoinCommand(server, handler);
		testPartCommand(server, handler);
		
		std::cout << "\n======================================\n";
		std::cout << "          All Tests Completed!         \n";
		std::cout << "======================================\n";
	
	} catch (const std::exception& e) {

		std::cerr << "\n!!! TEST CRASHED !!!\n";
		std::cerr << "Error: " << e.what() << "\n";
		return 1;
	}

	return 0;
}
