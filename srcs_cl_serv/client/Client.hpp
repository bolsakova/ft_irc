/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:40 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/16 23:39:25 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
	private:
	int			m_fd;
	std::string	m_inbuf;
	std::string	m_outbuf;
	// peer closed its write side (recv returned 0)
	bool		m_peer_closed;
	
	public:
	// deleted OCF methods (canonical but disabled)
	Client() = delete; //forbidden without fd
	Client(const Client& src) = delete;
	Client& operator=(const Client& rhs) = delete;
//explicit к констр. с одним параметром, запрещает неявные преобразования типов, от случайного вызова конструктора с int. 
//Особенно важно для классов, где аргумент — идентификатор ресурса (fd, socket, handler)	
	explicit Client(int fd);
	~Client();
	
	int getFD() const;
	void appendToInBuf(const std::string &data);
	bool hasCompleteCmd() const;
	std::string extractNextCmd();// вернет одну команду до \r\n
	const std::string& getInBuf() const;
	
	void appendToOutBuf(const std::string &data);
	bool hasDataToSend()const;
	const std::string& getOutBuf() const;
	void consumeOutBuf(std::size_t count);
	
	void markPeerClosed();
	bool isPeerClosed() const;
};

#endif