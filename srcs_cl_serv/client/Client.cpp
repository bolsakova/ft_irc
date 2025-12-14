/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:59 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/14 21:37:52 by aokhapki         ###   ########.fr       */
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
	if (pos == std::string::npos)
		return "";
	// Берем все до \n
	std::string cmd = m_inbuf.substr(0, pos);
	//усли перед \n стоит \r, удаляем его
	if (pos > 0 && cmd[pos - 1] == '\r')
		cmd.erase(pos - 1, 1);			
	m_inbuf.erase(0, pos + 2); // +2 to remove \r\n
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
