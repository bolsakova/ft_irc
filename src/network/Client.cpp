#include "network/Client.hpp"

Client::Client(int fd)
	: m_fd(fd),
	  m_inbuf(""),
	  m_outbuf(""),
	  m_nickname(""),
	  m_username(""),
	  m_realname(""),
	  m_authenticated(false),
	  m_registered(false),
	  m_peer_closed(false),
	  m_should_disconnect(false),	// init disconnect flag as false
	  m_quit_reason(""),			// no quit reason until requested
	  m_user_modes("")				// user modes start empty
{}

Client::~Client() {}

int Client::getFD() const { return m_fd; }

// = Name getters/setters =
void Client::setNickname(const std::string& nickname){m_nickname = nickname;}

void Client::setUsername(const std::string& username){m_username = username;}

void Client::setRealname(const std::string& realname){m_realname = realname;}

const std::string& Client::getNickname() const{return m_nickname;}

const std::string& Client::getUsername() const{return m_username;}

const std::string& Client::getRealname() const{return m_realname;}

// = Authentication and Registration state  =
void Client::setAuthenticated(bool auth){m_authenticated = auth;}

bool Client::isAuthenticated() const{return m_authenticated;}

void Client::setRegistered(bool reg){m_registered = reg;}

bool Client::isRegistered() const{return m_registered;}

void Client::appendToInBuf(const std::string &data){m_inbuf += data;}

/*
	Accept both IRC-style CRLF (\r\n) and plain LF (\n) as line endings.
	CRLF = Carriage Return + Line Feed (standard for IRC); LF = Line Feed (common in netcat/manual input).
*/
bool Client::hasCompleteCmd() const
{
	return m_inbuf.find("\r\n") != std::string::npos ||
		   m_inbuf.find('\n')   != std::string::npos;
}

std::string Client::extractNextCmd()
{
	std::size_t pos = m_inbuf.find("\r\n");
	std::size_t len = 2;
	if (pos == std::string::npos)
	{
		pos = m_inbuf.find('\n');
		len = 1;
	}
	if (pos == std::string::npos)
		return "";

	std::string line = m_inbuf.substr(0, pos);
	// Trim trailing '\r' if the delimiter was just '\n'
	if (len == 1 && !line.empty() && line[line.size() - 1] == '\r')
		line.erase(line.size() - 1, 1);
	m_inbuf.erase(0, pos + len);
	return line;
}

void Client::appendToOutBuf(const std::string &data){m_outbuf += data;}

bool Client::hasDataToSend() const{return !m_outbuf.empty();}

const std::string& Client::getOutBuf() const{return m_outbuf;}

void Client::consumeOutBuf(std::size_t count)
{
	if (count >= m_outbuf.size())
	{
		m_outbuf.clear();
		return;
	}
	m_outbuf.erase(0, count);
}

const std::string& Client::getInBuf() const{return m_inbuf;}

void Client::markPeerClosed(){m_peer_closed = true;}

bool Client::isPeerClosed() const{return m_peer_closed;}

/* 
 marks client for later disconnection by server(for QUIT cmd). Server will check shouldDisconnect() 
 in main loop and properly disconnect the client after processing current cmd.
*/
void Client::markForDisconnect(const std::string& reason)
{
	m_should_disconnect = true; // signal server to drop connection soon
	m_quit_reason = reason;     // remember quit reason for notices/logs
}

// Check if client should be disconnected, true if client should be disconnected, false otherwise
bool Client::shouldDisconnect() const{return m_should_disconnect;}
		
//Get disconnection reason (for logging/notifications), return Quit reason string
const std::string& Client::getQuitReason() const{return m_quit_reason;}

/**
 * @brief Set or unset a user mode
 * @param mode Mode character (i, o, w, etc.)
 * @param add True to add mode, false to remove
 */
void Client::setUserMode(char mode, bool add)
{
	if (add)
	{
		// Add mode if not already present
		if (m_user_modes.find(mode) == std::string::npos)
			m_user_modes += mode;
	}
	else
	{
		// Remove mode if present
		size_t pos = m_user_modes.find(mode);
		if (pos != std::string::npos)
			m_user_modes.erase(pos, 1);
	}
}

/**
 * @brief Check if user has a specific mode
 * @param mode Mode character to check
 * @return True if user has the mode
 */
bool Client::hasUserMode(char mode) const
{
	return m_user_modes.find(mode) != std::string::npos;
}
