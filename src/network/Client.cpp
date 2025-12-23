/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:59 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/23 15:50:39 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/network/Client.hpp"

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
	  m_should_disconnect(false), // init disconnect flag as false
	  m_quit_reason("") // no quit reason until requested
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

// =  IRC строго конец команды это "\r\n" =
bool Client::hasCompleteCmd() const{return (m_inbuf.find("\r\n") != std::string::npos);}

std::string Client::extractNextCmd()
{
	// режем по "\r\n", не по '\n'
	std::size_t pos = m_inbuf.find("\r\n");
	if (pos == std::string::npos)
		return "";

	std::string line = m_inbuf.substr(0, pos);
	m_inbuf.erase(0, pos + 2); // +2 because "\r\n" is 2 chars
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

//marks client for later disconnection by server(for QUIT cmd). Server will check shouldDisconnect() 
//in main loop and properly disconnect the client after processing current cmd.

void Client::markForDisconnect(const std::string& reason)
{
	m_should_disconnect = true; // signal server to drop connection soon
	m_quit_reason = reason;     // remember quit reason for notices/logs
}

// Check if client should be disconnected, true if client should be disconnected, false otherwise
bool Client::shouldDisconnect() const{return m_should_disconnect;}
		
//Get disconnection reason (for logging/notifications), return Quit reason string
const std::string& Client::getQuitReason() const{return m_quit_reason;}


