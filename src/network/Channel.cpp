#include "network/Channel.hpp"
#include "network/Client.hpp"
#include <iostream>

// Default constructor: empty channel with all modes disabled.
Channel::Channel()
	: m_name(),
	  m_topic(),
	  m_key(),
	  m_members(),
	  m_operators(),
	  m_invited(),
	  m_invite_only(false),
	  m_topic_protected(false),
	  m_user_limit(0)
{}

Channel::Channel(const Channel& src)
	: m_name(src.m_name),
	  m_topic(src.m_topic),
	  m_key(src.m_key),
	  m_members(src.m_members),
	  m_operators(src.m_operators),
	  m_invited(src.m_invited),
	  m_invite_only(src.m_invite_only),
	  m_topic_protected(src.m_topic_protected),
	  m_user_limit(src.m_user_limit)
{}

Channel& Channel::operator=(const Channel& rhs)
{
	if (this != &rhs)
	{
		m_name = rhs.m_name;
		m_topic = rhs.m_topic;
		m_key = rhs.m_key;
		m_members = rhs.m_members;
		m_operators = rhs.m_operators;
		m_invited = rhs.m_invited;
		m_invite_only = rhs.m_invite_only;
		m_topic_protected = rhs.m_topic_protected;
		m_user_limit = rhs.m_user_limit;
	}
	return *this;
}

Channel::~Channel() {}

// Set channel topic string visible to members.
void Channel::setTopic(const std::string& topic){m_topic = topic;}

// Check whether a topic is set (non-empty string).
bool Channel::hasTopic() const{return !m_topic.empty();}

// Return channel name so the server can route by it.
const std::string& Channel::getName() const{return m_name;}

// Read current topic for replies.
const std::string& Channel::getTopic() const{return m_topic;}

/*
  Add a member using its fd from Client and store in the map.
  If fd already exists, update the pointer (covers reconnect cases).
*/ 
void Channel::addMember(Client* client)
{
	if (!client)
		return;
	m_members[client->getFD()] = client;
}

// Remove a member by fd (used for PART/QUIT) and drop operator rights if present.
void Channel::removeMember(int fd)
{
	m_members.erase(fd);
	m_operators.erase(fd);
}

// Check whether fd is in the member list.
bool Channel::isMember(int fd) const{return m_members.find(fd) != m_members.end();}

// Return full member map (fd -> Client*) for iteration/broadcasts.
const std::map<int, Client*>& Channel::getMembers() const{return m_members;}

// Check whether the channel is empty.
bool Channel::isEmpty() const{return m_members.empty();}

// Grant operator status by adding fd to the operator set.
void Channel::addOperator(int fd){m_operators.insert(fd);}

// Revoke operator status for the given fd.
void Channel::removeOperator(int fd){m_operators.erase(fd);}

// Check whether a user is an operator of the channel.
bool Channel::isOperator(int fd) const{return m_operators.find(fd) != m_operators.end();}

// Set user limit +l (0 or less means no limit).
void Channel::setUserLimit(int limit){m_user_limit = limit;}

// Toggle invite-only mode +i.
void Channel::setInviteOnly(bool enable){m_invite_only = enable;}

// Toggle topic protection +t (only operators may change it).
void Channel::setTopicProtected(bool enable){m_topic_protected = enable;}

// Set channel key (password) for mode +k.
void Channel::setKey(const std::string& key){m_key = key;}

// Clear the channel key (equivalent to -k).
void Channel::removeKey(){m_key.clear();}

// Check whether +i is enabled.
bool Channel::isInviteOnly() const{return m_invite_only;}

// Check whether +t is enabled.
bool Channel::isTopicProtected() const{return m_topic_protected;}

// Check whether a key is set (non-empty string).
bool Channel::hasKey() const{return !m_key.empty();}

// Get current channel key.
const std::string& Channel::getKey() const{return m_key;}

// Get current user limit.
int Channel::getUserLimit() const{return m_user_limit;}

// Add user fd to the invited set (for +i).
void Channel::addInvited(int fd){m_invited.insert(fd);}

// Check whether a user has an invite.
bool Channel::isInvited(int fd) const{return m_invited.find(fd) != m_invited.end();}

// Remove a user from the invited set (after join or revoke).
void Channel::removeInvited(int fd){m_invited.erase(fd);}

// Broadcast to all members, optionally excluding sender by fd.
void Channel::broadcast(const std::string& message, int exclude_fd)
{
	std::cout << "Broadcasting to " << m_members.size() << " members" << std::endl;
    for (std::map<int, Client*>::iterator it = m_members.begin();
         it != m_members.end(); ++it)
    {
        if (it->first == exclude_fd) {
            std::cout << "  Skipping fd " << it->first << std::endl;
            continue;
        }
        std::cout << "  Sending to fd " << it->first << " (" 
                  << it->second->getNickname() << ")" << std::endl;
        it->second->appendToOutBuf(message);
    }
}
