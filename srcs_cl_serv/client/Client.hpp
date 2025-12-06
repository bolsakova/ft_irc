/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Client.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/05 17:44:40 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/06 09:46:06 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client
{
private:
	int m_fd;
	std::string m_inbuf;
	std::string m_outbuf;
	
public:
	// Deleted OCF methods (canonical but disabled)
	Client() = delete;                        // нельзя без fd
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
//explicit к констр. с одним параметром, запрещает неявные преобразования типов, от случайного вызова конструктора с int. 
//Особенно важно для классов, где аргумент — идентификатор ресурса (fd, socket, handler)	
	explicit Client(int fd);
	~Client();
	
	int getFD() const;
	void appendToInBuf(const std::string &data);
	bool hasCompleteCmd() const;
	std::string extractNextCmd();// вернет одну команду до \r\n
};

#endif
