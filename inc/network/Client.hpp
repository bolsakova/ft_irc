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
		std::string	m_inbuf;    // Partial commands
		std::string	m_outbuf;   // Data to send
		// IRC protocol state
		std::string m_nickname;
		std::string m_username;
		std::string m_realname;
		bool 		m_authenticated;
		bool 		m_registered;
		bool		m_peer_closed;       // peer closed its write side (recv returned 0)
		bool        m_should_disconnect; // Should server disconnect this client?
		std::string m_quit_reason;       // Reason for disconnection (for QUIT)
		std::string m_user_modes;        // User modes (i, o, w, etc.)
	
	public:
		// deleted OCF methods (canonical but disabled): Client manages a unique fd
		Client() = delete;                    // forbidden without fd: but fd is mandatory in this project
		Client(const Client& src) = delete;   
		Client& operator=(const Client& rhs) = delete;
		// explicit ctor prevents implicit conversions, important with fd/socket handles
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

		// Additional helper to clear input buffer (if needed) 30.12.2025
		// void clearInBuf() { m_inbuf.clear(); }

		// User mode management
		const std::string& getUserModes() const { return m_user_modes; }
		void setUserMode(char mode, bool add);
		bool hasUserMode(char mode) const;
};
#endif
