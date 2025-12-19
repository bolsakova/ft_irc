/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:59 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/16 23:39:55 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/network/Client.hpp"

Client::Client(int fd)
	: m_fd(fd), m_inbuf(""), m_outbuf(""), m_peer_closed(false) // init flag
{}

Client::~Client(){}

int Client::getFD() const
{
	return m_fd;
}

void Client::appendToInBuf(const std::string &data)
{
	m_inbuf += data;
}

bool Client::hasCompleteCmd() const
{
	// for IRC строго конец команды это "\r\n"
	return (m_inbuf.find("\r\n") != std::string::npos);
}

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

void Client::appendToOutBuf(const std::string &data)
{
	m_outbuf += data;
}

bool Client::hasDataToSend() const
{
	return !m_outbuf.empty();
}

const std::string& Client::getOutBuf() const
{
	return m_outbuf;
}

void Client::consumeOutBuf(std::size_t count)
{
	if (count >= m_outbuf.size())
	{
		m_outbuf.clear();
		return;
	}
	m_outbuf.erase(0, count);
}

const std::string& Client::getInBuf() const
{
	return m_inbuf;
}

// NEW
void Client::markPeerClosed()
{
	m_peer_closed = true;
}

bool Client::isPeerClosed() const
{
	return m_peer_closed;
}
