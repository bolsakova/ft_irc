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
		// IRC protocol state
		std::string m_nickname;
		std::string m_username;
		std::string m_realname;
		bool 		m_authenticated;
		bool 		m_registered;
		// peer closed its write side (recv returned 0)
		bool		m_peer_closed;
		// вisconnect management
		bool        m_should_disconnect; // Should server disconnect this client?
		std::string m_quit_reason;       // Reason for disconnection (for QUIT)

	
	public:
		// deleted OCF methods (canonical but disabled): Client manages a unique fd
		Client() = delete;                    // forbidden without fd: but fd is mandatory in this project
		Client(const Client& src) = delete;   
		Client& operator=(const Client& rhs) = delete;
		// explicit constr prevents implicit(неявное) type conversions, important with fd, socket, handle
		explicit Client(int fd);
		~Client();

		// = Client identity =
		int getFD() const;

		// = Name getters/setters =
		void setNickname(const std::string& nickname);
		void setUsername(const std::string& username);
		void setRealname(const std::string& realname);
		const std::string& getNickname() const;
		const std::string& getUsername() const;
		const std::string& getRealname() const;
		
		// = Authentication and Registration state =
		void setAuthenticated(bool auth);
		bool isAuthenticated() const;
		void setRegistered(bool reg);
		bool isRegistered() const;

		// = Incoming data handling (input buffer) =
		void appendToInBuf(const std::string &data);
		bool hasCompleteCmd() const;
		std::string extractNextCmd();          // extracts one command ending with \r\n
		const std::string& getInBuf() const;

		// = Outgoing data handling (output buffer) =
		void appendToOutBuf(const std::string &data);
		const std::string& getOutBuf() const;
		void consumeOutBuf(std::size_t count);
		bool hasDataToSend() const;

		// = Connection state =
		void markPeerClosed();
		bool isPeerClosed() const;
		void markForDisconnect(const std::string& reason);
		bool shouldDisconnect() const;
		const std::string& getQuitReason() const;
};

#endif