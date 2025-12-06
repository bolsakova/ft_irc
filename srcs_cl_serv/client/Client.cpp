/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:59 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/06 09:39:29 by aokhapki         ###   ########.fr       */
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
	return m_inbuf.find("\r\n") != std::string::npos;
}

std::string Client::extractNextCmd()
{
	size_t pos = m_inbuf.find("\r\n");
	if (pos == std::string::npos)
		return "";

	std::string cmd = m_inbuf.substr(0, pos);
	m_inbuf.erase(0, pos + 2); // +2 to remove \r\n
	return cmd;
}