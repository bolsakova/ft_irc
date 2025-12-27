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

void testKickCommand(Server& server, CommandHandler& handler)
{
	std::cout << "\n=== Testing KICK Command ===\n";

	// Test 1: KICK without registration
	std::cout << "\nTest 1: KICK without registration\n";
	Client unregistered(500);
	unregistered.setRegistered(false);

	handler.handleCommand("KICK #test user1\r\n", unregistered);
	std::string response = unregistered.getOutBuf();

	bool hasError = (response.find("451") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTREGISTERED", hasError);

	// Test 2: KICK without channel parameter
	std::cout << "\nTest 2: KICK without channel parameter\n";
	Client registered(501);
	registered.setAuthenticated(true);
	registered.setNickname("operator");
	registered.setUsername("operator");
	registered.setRegistered(true);

	handler.handleCommand("KICK\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("461") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NEEDMOREPARAMS", hasError);

	// Test 3: KICK without user parameter
	std::cout << "\nTest 3: KICK without user parameter\n";
	registered.consumeOutBuf(registered.getOutBuf().size());

	handler.handleCommand("KICK #test\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("461") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NEEDMOREPARAMS", hasError);

	// Test 4: KICK from non-existent channel
	std::cout << "\nTest 4: KICK from non-existent channel\n";
	registered.consumeOutBuf(registered.getOutBuf().size());

	handler.handleCommand("KICK #nonexistent user1\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("403") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHCHANNEL", hasError);

	// Test 5: KICK when not on channel
	std::cout << "\nTest 5: KICK when not on channel\n";

	// Create channel with another user
	Client chanOp(502);
	chanOp.setAuthenticated(true);
	chanOp.setNickname("chanop");
	chanOp.setUsername("chanop");
	chanOp.setRegistered(true);

	handler.handleCommand("JOIN #kicktest\r\n", chanOp);
	chanOp.consumeOutBuf(chanOp.getOutBuf().size());
	
	// Try to KICK from someone not on channel
	Client outsider(503);
	outsider.setAuthenticated(true);
	outsider.setNickname("outsider");
	outsider.setUsername("outsider");
	outsider.setRegistered(true);
	
	handler.handleCommand("KICK #kicktest chanop\r\n", outsider);
	response = outsider.getOutBuf();

	hasError = (response.find("442") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTONCHANNEL", hasError);

	// Test 6: KICK when not operator
	std::cout << "\nTest 6: KICK when not operator\n";

	// Join another user to channel (not operator)
	Client normalUser(504);
	normalUser.setAuthenticated(true);
	normalUser.setNickname("normaluser");
	normalUser.setUsername("normaluser");
	normalUser.setRegistered(true);

	handler.handleCommand("JOIN #kicktest\r\n", normalUser);
	normalUser.consumeOutBuf(normalUser.getOutBuf().size());
	
	// Normal user tries to kick operator
	handler.handleCommand("KICK #kicktest chanop\r\n", normalUser);
	response = normalUser.getOutBuf();

	hasError = (response.find("482") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_CHANOPRIVSNEEDED", hasError);

	// Test 7: KICK user not on channel
	std::cout << "\nTest 7: KICK user not on channel\n";
	chanOp.consumeOutBuf(chanOp.getOutBuf().size());

	handler.handleCommand("KICK #kicktest nonexistentuser\r\n", chanOp);
	response = chanOp.getOutBuf();

	hasError = (response.find("441") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_USERNOTINCHANNEL", hasError);

	// Test 8: Successful KICK without reason
	std::cout << "\nTest 8: Successful KICK without reason\n";

	Client kickOp(505);
	kickOp.setAuthenticated(true);
	kickOp.setNickname("kickop");
	kickOp.setUsername("kickop");
	kickOp.setRegistered(true);

	Client victim(506);
	victim.setAuthenticated(true);
	victim.setNickname("victim");
	victim.setUsername("victim");
	victim.setRegistered(true);

	handler.handleCommand("JOIN #testkick\r\n", kickOp);
	handler.handleCommand("JOIN #testkick\r\n", victim);
	
	kickOp.consumeOutBuf(kickOp.getOutBuf().size());
	victim.consumeOutBuf(victim.getOutBuf().size());
	
	// Operator kicks victim
	handler.handleCommand("KICK #testkick victim\r\n", kickOp);

	std::string opResponse = kickOp.getOutBuf();
	std::string victimResponse = victim.getOutBuf();
	
	bool hasKick = (opResponse.find("KICK") != std::string::npos);
	bool hasChannel = (opResponse.find("#testkick") != std::string::npos);
	bool hasVictim = (opResponse.find("victim") != std::string::npos);
	
	std::cout << "  Has KICK: " << (hasKick ? "yes" : "no") << "\n";
	std::cout << "  Has channel: " << (hasChannel ? "yes" : "no") << "\n";
	std::cout << "  Has victim: " << (hasVictim ? "yes" : "no") << "\n";

	printTestResult("KICK without reason", hasKick && hasChannel && hasVictim);
	
	// Verify victim received KICK
	bool victimGotKick = (victimResponse.find("KICK") != std::string::npos);
	std::cout << "  Victim got KICK: " << (victimGotKick ? "yes" : "no") << "\n";
	printTestResult("Victim received KICK", victimGotKick);
	
	// Verify victim is removed from channel
	Channel* chan = server.findChannel("#testkick");
	bool removed = (!chan || !chan->isMember(506));
	std::cout << "  Victim removed: " << (removed ? "yes" : "no") << "\n";
	printTestResult("Victim removed from channel", removed);
	
	// Test 9: Successful KICK with reason
	std::cout << "\nTest 9: Successful KICK with reason\n";

	Client kickOp2(507);
	kickOp2.setAuthenticated(true);
	kickOp2.setNickname("kickop2");
	kickOp2.setUsername("kickop2");
	kickOp2.setRegistered(true);

	Client victim2(508);
	victim2.setAuthenticated(true);
	victim2.setNickname("victim2");
	victim2.setUsername("victim2");
	victim2.setRegistered(true);

	handler.handleCommand("JOIN #reasonkick\r\n", kickOp2);
	handler.handleCommand("JOIN #reasonkick\r\n", victim2);
	
	kickOp2.consumeOutBuf(kickOp2.getOutBuf().size());
	victim2.consumeOutBuf(victim2.getOutBuf().size());
	
	// Kick with reason
	handler.handleCommand("KICK #reasonkick victim2 :Bad behavior\r\n", kickOp2);

	opResponse = kickOp2.getOutBuf();
	victimResponse = victim2.getOutBuf();
	
	hasKick = (opResponse.find("KICK") != std::string::npos);
	bool hasReason = (opResponse.find("Bad behavior") != std::string::npos);
	
	std::cout << "  Has KICK: " << (hasKick ? "yes" : "no") << "\n";
	std::cout << "  Has reason: " << (hasReason ? "yes" : "no") << "\n";

	printTestResult("KICK with reason", hasKick && hasReason);
	
	// Verify victim received reason
	bool victimGotReason = (victimResponse.find("Bad behavior") != std::string::npos);
	std::cout << "  Victim got reason: " << (victimGotReason ? "yes" : "no") << "\n";
	printTestResult("Victim received reason", victimGotReason);
	
	// Test 10: KICK broadcasts to all channel members
	std::cout << "\nTest 10: KICK broadcasts to all channel members\n";

	Client op(509);
	op.setAuthenticated(true);
	op.setNickname("op");
	op.setUsername("op");
	op.setRegistered(true);

	Client user1(510);
	user1.setAuthenticated(true);
	user1.setNickname("user1");
	user1.setUsername("user1");
	user1.setRegistered(true);

	Client user2(511);
	user2.setAuthenticated(true);
	user2.setNickname("user2");
	user2.setUsername("user2");
	user2.setRegistered(true);

	// All join channel
	handler.handleCommand("JOIN #broadcast\r\n", op);
	handler.handleCommand("JOIN #broadcast\r\n", user1);
	handler.handleCommand("JOIN #broadcast\r\n", user2);
	
	op.consumeOutBuf(op.getOutBuf().size());
	user1.consumeOutBuf(user1.getOutBuf().size());
	user2.consumeOutBuf(user2.getOutBuf().size());
	
	// Op kicks user1
	handler.handleCommand("KICK #broadcast user1 :Kicked!\r\n", op);

	std::string opOut = op.getOutBuf();
	std::string user1Out = user1.getOutBuf();
	std::string user2Out = user2.getOutBuf();
	
	bool opSeesKick = (opOut.find("KICK") != std::string::npos);
	bool user1SeesKick = (user1Out.find("KICK") != std::string::npos);
	bool user2SeesKick = (user2Out.find("KICK") != std::string::npos);
	
	std::cout << "  Op sees KICK: " << (opSeesKick ? "yes" : "no") << "\n";
	std::cout << "  User1 sees KICK: " << (user1SeesKick ? "yes" : "no") << "\n";
	std::cout << "  User2 sees KICK: " << (user2SeesKick ? "yes" : "no") << "\n";

	printTestResult("KICK broadcasts to all members", opSeesKick && user1SeesKick && user2SeesKick);
	
	std::cout << "=== KICK Command Tests Complete ===\n";
}

void testInviteCommand(Server& server, CommandHandler& handler)
{
	std::cout << "\n=== Testing INVITE Command ===\n";

	// Test 1: INVITE without registration
	std::cout << "\nTest 1: INVITE without registration\n";
	Client unregistered(600);
	unregistered.setRegistered(false);

	handler.handleCommand("INVITE user1 #test\r\n", unregistered);
	std::string response = unregistered.getOutBuf();

	bool hasError = (response.find("451") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTREGISTERED", hasError);

	// Test 2: INVITE without nickname parameter
	std::cout << "\nTest 2: INVITE without nickname parameter\n";
	Client registered(601);
	registered.setAuthenticated(true);
	registered.setNickname("inviter");
	registered.setUsername("inviter");
	registered.setRegistered(true);

	handler.handleCommand("INVITE\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("461") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NEEDMOREPARAMS", hasError);

	// Test 3: INVITE without channel parameter
	std::cout << "\nTest 3: INVITE without channel parameter\n";
	registered.consumeOutBuf(registered.getOutBuf().size());

	handler.handleCommand("INVITE user1\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("461") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NEEDMOREPARAMS", hasError);

	// Test 4: INVITE to non-existent channel
	std::cout << "\nTest 4: INVITE to non-existent channel\n";
	registered.consumeOutBuf(registered.getOutBuf().size());

	handler.handleCommand("INVITE user1 #nonexistent\r\n", registered);
	response = registered.getOutBuf();

	hasError = (response.find("403") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHCHANNEL", hasError);

	// Test 5: INVITE when not on channel
	std::cout << "\nTest 5: INVITE when not on channel\n";

	// Create channel with another user
	Client chanOp(602);
	chanOp.setAuthenticated(true);
	chanOp.setNickname("chanop");
	chanOp.setUsername("chanop");
	chanOp.setRegistered(true);

	handler.handleCommand("JOIN #invitetest\r\n", chanOp);
	chanOp.consumeOutBuf(chanOp.getOutBuf().size());
	
	// Try to INVITE from someone not on channel
	Client outsider(603);
	outsider.setAuthenticated(true);
	outsider.setNickname("outsider");
	outsider.setUsername("outsider");
	outsider.setRegistered(true);
	
	handler.handleCommand("INVITE chanop #invitetest\r\n", outsider);
	response = outsider.getOutBuf();

	hasError = (response.find("442") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOTONCHANNEL", hasError);

	// Test 6: INVITE on +i when not operator
	std::cout << "\nTest 6: INVITE on +i when not operator\n";

	// Create invite-only channel
	Channel* inviteChan = server.createChannel("#inviteonly");
	inviteChan->setInviteOnly(true);

	Client op(604);
	op.setAuthenticated(true);
	op.setNickname("op");
	op.setUsername("op");
	op.setRegistered(true);

	Client normalUser(605);
	normalUser.setAuthenticated(true);
	normalUser.setNickname("normaluser");
	normalUser.setUsername("normaluser");
	normalUser.setRegistered(true);

	// Op creates channel (becomes operator)
	inviteChan->addMember(&op);
	inviteChan->addOperator(op.getFD());

	// Normal user joins (need to bypass +i for test setup)
	inviteChan->addInvited(normalUser.getFD());
	inviteChan->addMember(&normalUser);

	// Normal user tries to invite someone
	handler.handleCommand("INVITE guest #inviteonly\r\n", normalUser);
	response = normalUser.getOutBuf();

	hasError = (response.find("482") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_CHANOPRIVSNEEDED", hasError);

	// Test 7: INVITE non-existent user
	std::cout << "\nTest 7: INVITE non-existent user\n";

	Client inviter(606);
	inviter.setAuthenticated(true);
	inviter.setNickname("inviter");
	inviter.setUsername("inviter");
	inviter.setRegistered(true);

	handler.handleCommand("JOIN #testinv\r\n", inviter);
	inviter.consumeOutBuf(inviter.getOutBuf().size());
	
	handler.handleCommand("INVITE nonexistent #testinv\r\n", inviter);
	response = inviter.getOutBuf();

	hasError = (response.find("401") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_NOSUCHNICK", hasError);

	// Test 8: INVITE user already on channel
	std::cout << "\nTest 8: INVITE user already on channel\n";

	Client member1(607);
	member1.setAuthenticated(true);
	member1.setNickname("member1");
	member1.setUsername("member1");
	member1.setRegistered(true);

	Client member2(608);
	member2.setAuthenticated(true);
	member2.setNickname("member2");
	member2.setUsername("member2");
	member2.setRegistered(true);

	// Both join channel
	handler.handleCommand("JOIN #testinv2\r\n", member1);
	handler.handleCommand("JOIN #testinv2\r\n", member2);
	
	member1.consumeOutBuf(member1.getOutBuf().size());
	member2.consumeOutBuf(member2.getOutBuf().size());
	
	// Try to invite member2 who is already on channel
	handler.handleCommand("INVITE member2 #testinv2\r\n", member1);
	response = member1.getOutBuf();

	hasError = (response.find("443") != std::string::npos);
	std::cout << "  Response: " << response;
	printTestResult("ERR_USERONCHANNEL", hasError);

	// Test 9: Successful INVITE
	std::cout << "\nTest 9: Successful INVITE\n";

	// Add clients to server
	Client* host = new Client(609);
	host->setAuthenticated(true);
	host->setNickname("host");
	host->setUsername("host");
	host->setRegistered(true);
	server.addClient(609, std::unique_ptr<Client>(host));

	Client* guest = new Client(610);
	guest->setAuthenticated(true);
	guest->setNickname("guest");
	guest->setUsername("guest");
	guest->setRegistered(true);
	server.addClient(610, std::unique_ptr<Client>(guest));

	// Host creates channel
	handler.handleCommand("JOIN #party\r\n", *host);
	host->consumeOutBuf(host->getOutBuf().size());
	
	// Host invites guest
	handler.handleCommand("INVITE guest #party\r\n", *host);

	std::string hostResponse = host->getOutBuf();
	std::string guestResponse = guest->getOutBuf();
	
	bool hasInviting = (hostResponse.find("341") != std::string::npos);
	bool hasInvite = (guestResponse.find("INVITE") != std::string::npos);
	bool hasChannel = (guestResponse.find("#party") != std::string::npos);
	
	std::cout << "  Host response: " << hostResponse;
	std::cout << "  Guest response: " << guestResponse;
	std::cout << "  Has RPL_INVITING (341): " << (hasInviting ? "yes" : "no") << "\n";
	std::cout << "  Guest got INVITE: " << (hasInvite ? "yes" : "no") << "\n";

	printTestResult("Successful INVITE", hasInviting && hasInvite && hasChannel);
	
	// Test 10: INVITE allows joining +i channel
	std::cout << "\nTest 10: INVITE allows joining +i channel\n";

	// Create invite-only channel
	// Channel* privateChan = server.createChannel("#private");
	// privateChan->setInviteOnly(true);

	Client* owner = new Client(611);
	owner->setAuthenticated(true);
	owner->setNickname("owner");
	owner->setUsername("owner");
	owner->setRegistered(true);
	server.addClient(611, std::unique_ptr<Client>(owner));

	Client* invited = new Client(612);
	invited->setAuthenticated(true);
	invited->setNickname("invited");
	invited->setUsername("invited");
	invited->setRegistered(true);
	server.addClient(612, std::unique_ptr<Client>(invited));

	// Owner creates +i channel
	handler.handleCommand("JOIN #private\r\n", *owner);
	Channel* privateChan = server.findChannel("#private");
	privateChan->setInviteOnly(true);
	
	owner->consumeOutBuf(owner->getOutBuf().size());
	invited->consumeOutBuf(invited->getOutBuf().size());
	
	// Owner invites user
	handler.handleCommand("INVITE invited #private\r\n", *owner);
	owner->consumeOutBuf(owner->getOutBuf().size());
	invited->consumeOutBuf(invited->getOutBuf().size());
	
	// Invited user can now join
	handler.handleCommand("JOIN #private\r\n", *invited);
	response = invited->getOutBuf();

	bool hasJoin = (response.find("JOIN") != std::string::npos);
	bool noError = (response.find("473") == std::string::npos);
	
	std::cout << "  Invited user response: " << response;
	std::cout << "  Has JOIN: " << (hasJoin ? "yes" : "no") << "\n";
	std::cout << "  No invite error: " << (noError ? "yes" : "no") << "\n";

	printTestResult("Invited user can join +i channel", hasJoin && noError);
	
	std::cout << "=== INVITE Command Tests Complete ===\n";
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
		// testPartCommand(server, handler);
		// testKickCommand(server, handler);
		testInviteCommand(server, handler);
		
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
