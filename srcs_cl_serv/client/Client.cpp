/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:59 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/15 00:11:02 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Client.hpp"

Client::Client(int fd)
	: m_fd(fd), m_inbuf(""), m_outbuf("")
{
}
Client::~Client()
{
	//fd will be closed by Server when removing the client Server::disconnectClient()
}
void Client::appendToInBuf(const std::string &data)
{
	m_inbuf += data;
}
bool Client::hasCompleteCmd() const
{
	return m_inbuf.find('\n') != std::string::npos;
}

std::string Client::extractNextCmd()
{
	size_t pos = m_inbuf.find('\n');
	std::string cmd = m_inbuf.substr(0, pos);
	if (!cmd.empty() && cmd[cmd.size() - 1] == '\r')
		cmd.erase(cmd.size() - 1);
	m_inbuf.erase(0, pos + 1);
	return cmd;
}
void Client::appendToOutBuf(const std::string &data)
{
	m_outbuf = m_outbuf + data; // append data to outbuf - concatenation of strings
}

bool Client::hasDataToSend() const
{
	return !m_outbuf.empty(); // 1 = есть данные, 0 - пуст
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
