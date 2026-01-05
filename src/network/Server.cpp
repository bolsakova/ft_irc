#include <cstring>        // strerror, memset
#include <iostream>       // cout, cerr
#include <cerrno>         // errno
#include <csignal>        // signal/sigaction (SIGPIPE)
#include <fcntl.h>        // fcntl (for non-blocking)
#include <unistd.h>       // close, read, write
#include <sys/socket.h>   // socket, bind, listen, accept
#include <netinet/in.h>   // sockaddr_in, htons
#include "network/Server.hpp"
#include "protocol/CommandHandler.hpp"

/*
EAGAIN/EWOULDBLOCK - no pending connect, non-block and interrupt: no crash -> temp no data, retry later.
POLLIN — data ready to read
POLLOUT — socket ready to write
revents returns:
POLLIN — data ready to read
POLLOUT — socket ready to write
POLLERR — error occurred
POLLHUP — connection hangup
fd(0, 1, 2 (stdin, stdout, stderr)
*/

/*
ignore SIGPIPE to prevent server crash on writing to closed socket.
*/
static void ignore_sigpipe()
{
	struct sigaction sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGPIPE, &sa, NULL);
}
/*
 	This function sets a file descriptor to non-blocking mode.
	Non-blocking is required for poll()/epoll() architecture without hanging/blocking or freezing the program.
	fcntl(fd, F_GETFL) -> get current flags
	fcntl(fd, F_SETFL) -> set new flags
*/
static int set_non_blocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return -1;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return -1;
	return 0;
}

/*
	Strictly parse port string to integer in range 1024-65535 (unprivileged).
	Throws runtime_error on invalid input.
	Strict validation instead of atoi()-(atoi("12abc") returns 12, 
	overflow, empty/space returns 0 or UB
*/
static int parse_port_strict(const std::string &port_str)
{
	if (port_str.empty())
		throw std::runtime_error("Invalid port number: (empty)");

	long port = 0;
	for (std::size_t i = 0; i < port_str.size(); ++i)
	{
		if (port_str[i] < '0' || port_str[i] > '9')
			throw std::runtime_error("Invalid port number: " + port_str);
		port = port * 10 + (port_str[i] - '0');
		if (port > 65535)
			throw std::runtime_error("Invalid port number: " + port_str);
	}
	if (port < 1024 || port > 65535)
		throw std::runtime_error("Invalid port number: " + port_str);
	return static_cast<int>(port);
}

/*
   Disable specified poll event (e.g., POLLIN) for given fd
*/ 
static void disable_pollevent(std::vector<pollfd> &poll_fds, int fd, short flag)
{
	for (size_t i = 0; i < poll_fds.size(); ++i)
	{
		if (poll_fds[i].fd == fd)
		{
			poll_fds[i].events = poll_fds[i].events & (~flag);
			return;
		}
	}
}

/*
	Constructor sets up the listening socket and poll tracking:
	- Ignore SIGPIPE to avoid crashing on write to closed sockets
	- initSocket(port) creates/binds/listens non-blocking on the requested port
	- After socket is ready, create CommandHandler
	- Register the listening fd in poll() with POLLIN
	- On any init failure, close the socket and rethrow to signal construction error
*/
Server::Server(const std::string& port, const std::string& password)
	: m_listen_fd(-1), m_running(true), m_password(password)
{
	ignore_sigpipe();
	try {
		initSocket(port);
		// Create CommandHandler after successful socket init
		m_cmd_handler = std::make_unique<CommandHandler>(*this, m_password);
		// Start tracking listening socket in poll()
		pollfd listen_pfd;
		listen_pfd.fd = m_listen_fd;
		listen_pfd.events = POLLIN;
		listen_pfd.revents = 0;
		m_poll_fds.push_back(listen_pfd);
	} catch (const std::exception& e) 
	{
		// Clean up socket if initialization fails
		if (m_listen_fd >= 0) {
			close(m_listen_fd);
			m_listen_fd = -1;
		}
		throw; 
	}
}

/*
 unique_ptr automatically frees memory. We do NOT need to delete Client manually.
 1. Close listening socket
 2. Close all client sockets
 3. Explicitly clear containers (optional but makes intent clear)
*/ 
Server::~Server()
{
	if (m_listen_fd >= 0)
		close(m_listen_fd);
	for (std::map<int, std::unique_ptr<Client>>::iterator it = m_clients.begin();
		 it != m_clients.end(); ++it)
	{
		if (it->first >= 0)
			close(it->first);
	}
	m_clients.clear();
	m_poll_fds.clear();
}

/*
  returns nullptr if channel not found
*/ 
Channel* Server::findChannel(const std::string& name)
{
	std::map<std::string, std::unique_ptr<Channel> >::iterator it = m_channels.find(name);
	if (it == m_channels.end())
		return NULL;
	return it->second.get();
}

Client* Server::findClientByNickname(const std::string& nickname)
{
	for (std::map<int, std::unique_ptr<Client>>::iterator it = m_clients.begin();
		it != m_clients.end(); ++it)
	{
		if (it->second->getNickname() == nickname)
			return it->second.get();
	}
	return NULL;
}

void Server::addClient(int fd, std::unique_ptr<Client> client)
{
	m_clients[fd] = std::move(client);
}

/*
  Create channel if it doesn't exist; return pointer to existing/new.
*/
Channel* Server::createChannel(const std::string& name)
{
	Channel* existing = findChannel(name);
	if (existing)
		return existing;
	std::unique_ptr<Channel> ch(new Channel());
	Channel* raw = ch.get();
	m_channels[name] = std::move(ch);
	return raw;
}

// Remove channel by name (if exists)
void Server::removeChannel(const std::string& name){m_channels.erase(name);}

// Get map of all channels (read-only access)
const std::map<std::string, std::unique_ptr<Channel>>& Server::getChannels() const{return m_channels;}

/*
IPv4 32-bit  AF_INET
IPv6 128-bit AF_INET6
Port 0: Reserved by the OS (means "let the system choose a port")
Linux: valid ports are 1-65535 (TCP/IP standard, 16-bit unsigned)
Ports 1-1023 require root/sudo privileges; use 1024+ for unprivileged apps
Convert port to integer and check validity
Create socket (AF_INET = IPv4, TCP)
Bind socket to the specified 0.0.0.0:port/sockaddr_in — struct from <netinet/in.h> for IPv4 socket address
SO_REUSEADDR - allow reusing the port after restart (without waiting ~60 seconds)
*/
void Server::initSocket(const std::string &port_str)
{
	int port;

	port = parse_port_strict(port_str);
	m_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_listen_fd < 0)
	throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
	int opt = 1; // 1 = enable SO_REUSEADDR
	if (setsockopt(m_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("setsockopt() failed: " + std::string(strerror(errno)));
	}
	sockaddr_in server_addr;
	std::memset(&server_addr, 0, sizeof(server_addr)); 
	server_addr.sin_family = AF_INET; 
	server_addr.sin_addr.s_addr = INADDR_ANY;// address in network byte order 
	server_addr.sin_port = htons(port);// port in network byte order

	if (bind(m_listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("bind() failed: " + std::string(strerror(errno)));
	}
	// 5. Start listening incoming connections non-blocking mode
	if (listen(m_listen_fd, SOMAXCONN) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
	}
	// 6. Make socket NON-BLOCKING, mandatory for poll() 
	if (set_non_blocking(m_listen_fd) < 0)
	{
		close(m_listen_fd);
		throw std::runtime_error("set_non_blocking() failed: " + std::string(strerror(errno)));
	}
	// std::cout << "Listening on port " << port << " (non-blocking)" << std::endl;
}


/*
	Accept new incoming connections on the listening socket:
	- Loop accept() until EAGAIN/EWOULDBLOCK (non-blocking listener)
	- On error: log and stop processing
	- Set each client socket to non-blocking
	- Add new fd to poll list with POLLIN
	- Create Client object for the new connection
*/
void Server::acceptClient()
{
	while (true)
	{
		sockaddr_in client_addr;
		socklen_t client_len;
		
		client_len = sizeof(client_addr);
		int client_fd = accept(m_listen_fd, (sockaddr *)&client_addr, &client_len);
		if (client_fd < 0)
		{
			// accept() on non-blocking socket: returns -1 when no pending connections - EAGAIN/EWOULDBLOCK means: no more pending connections (normal)
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			// any other error — log it
			std::cerr << "accept() failed: " << std::strerror(errno) << "\n";
			break;
		}
		if (set_non_blocking(client_fd) < 0)// Set client socket non-blocking
		{
			std::cerr << "Failed to set client non-blocking, fd " << client_fd << "\n";
			close(client_fd);
			continue;
		}
		// Add to poll list
		pollfd pfd;
		pfd.fd = client_fd;
		pfd.events = POLLIN;  // start with only read events
		pfd.revents = 0;
		m_poll_fds.push_back(pfd);//push_back копирует структуру pollfd и добавляет в вектор
		m_clients.emplace(client_fd, std::make_unique<Client>(client_fd)); //without copy constructor
		// std::cout << "New client accepted, fd = " << client_fd << std::endl;
	}
}

/*
 Remove fd from poll fds
 Close socket (free OS resource)
 Remove from clients map (unique_ptr frees memory)
*/
void Server::disconnectClient(int fd)
{
	for (size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		if (m_poll_fds[i].fd == fd)
		{
			m_poll_fds.erase(m_poll_fds.begin() + i);
			break;
		}
	}
	if (fd >= 0)
		close(fd);

	m_clients.erase(fd);
	// std::cout << "Client fd " << fd << " disconnected and removed." << std::endl;
}

/*
	Iterate through all tracked descriptors
	Find the target fd
	Add POLLOUT flag so poll() waits for write-ready
	Exit after updating the target descriptor
*/
void Server::enablePolloutForFD(int fd)
{
	for(size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		if(m_poll_fds[i].fd == fd)
		{
			m_poll_fds[i].events = m_poll_fds[i].events | POLLOUT;
			return;
		}
	}
}

/*
 Iterate through all tracked descriptors
 Find the target fd
 Remove POLLOUT flag so poll() no longer waits for write-ready
 Exit after updating the target descriptor
*/
void Server::disablePolloutForFd(int fd)
{
	for(size_t i = 0; i < m_poll_fds.size(); ++i)
	{
		if(m_poll_fds[i].fd == fd)
		{
			m_poll_fds[i].events = m_poll_fds[i].events & (~POLLOUT);
			return;
		}
	}
}

void Server::cleanupDisconnectedClients()
{
	if (m_clients.empty())
		return;
	std::vector<int> to_disconnect;
	to_disconnect.reserve(m_clients.size());
	for (std::map<int, std::unique_ptr<Client>>::const_iterator it = m_clients.begin();
		 it != m_clients.end(); ++it)
	{
		if (it->second->shouldDisconnect())
			to_disconnect.push_back(it->first);
	}
	for (size_t i = 0; i < to_disconnect.size(); ++i)
		disconnectClient(to_disconnect[i]);
}

/* Partial command handling — proper non-blocking receive loop
 4096 is a common chunk size
 Read repeatedly until EAGAIN/EWOULDBLOCK
 On error: log and drop client
 On 0 bytes (EOF/Ctrl+D): stop POLLIN, mark peer closed, disconnect after flushing pending output
 On data: append to per-client buffer (with size guard) and process all complete commands (CRLF/LF)
*/
bool Server::receiveData(int fd)
{
	char buffer[4096];
	ssize_t bytes_read;
	while (true)
	{
		bytes_read = recv(fd, buffer, sizeof(buffer), 0);
		if (bytes_read < 0)
		{
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			std::cerr << "recv() failed on fd " << fd << ": "
					  << std::strerror(errno) << std::endl;
			disconnectClient(fd);
			return false;
		}
		if (bytes_read == 0)
		{
			// std::cout << "Client fd " << fd << " closed input (EOF).\n";
			auto it = m_clients.find(fd);
			if (it == m_clients.end())
				return false;
			Client &client = *(it->second);
			client.markPeerClosed();
			disable_pollevent(m_poll_fds, fd, POLLIN);
			if (!client.hasDataToSend())
			{
				disconnectClient(fd);
				return false;
			}
			return true;
		}
		auto it = m_clients.find(fd);
		if (it == m_clients.end())
			return false;

		Client &client = *(it->second);

		std::string data(buffer, static_cast<std::size_t>(bytes_read));
		const std::size_t MAX_INBUF = 8192;
		if (client.getInBuf().size() + data.size() > MAX_INBUF)
		{
			std::cerr << "Input buffer overflow for fd " << fd
					  << " (limit " << MAX_INBUF << ")\n";
			disconnectClient(fd);
			return false;
		}
		client.appendToInBuf(data);
		while (client.hasCompleteCmd())
		{
			std::string cmd = client.extractNextCmd();
			m_cmd_handler->handleCommand(cmd, client);
			if (client.hasDataToSend())
				enablePolloutForFD(fd);
		}
	}
	return true;
}

const std::map<int, std::unique_ptr<Client>>& Server::getClients() const
{
	return m_clients;
}

/* 
#ifdef MSG_NOSIGNAL
	flags = MSG_NOSIGNAL;
 Protect from SIGPIPE on Linux with MSG_NOSIGNAL (extra safety).
 We still ignore SIGPIPE globally, but this makes send() itself safer.
*/
void Server::sendData(int fd)
{
	auto it = m_clients.find(fd);
	if (it == m_clients.end())
		return;
	Client &client = *(it->second);
	if (!client.hasDataToSend())
	{
		disablePolloutForFd(fd);
		return;
	}
	const std::string &out = client.getOutBuf();

	int flags = 0;

	#ifdef MSG_NOSIGNAL
		flags = MSG_NOSIGNAL;
	#endif
	ssize_t sent = send(fd, out.c_str(), out.size(), flags);
	if (sent < 0)
	{	
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		std::cerr << "send() failed on fd " << fd << ": "
				  << std::strerror(errno) << std::endl;
		disconnectClient(fd);
		return;
	}
	client.consumeOutBuf(static_cast<std::size_t>(sent));
	if (!client.hasDataToSend())
		disablePolloutForFd(fd);
	if (client.isPeerClosed() && !client.hasDataToSend())
		disconnectClient(fd);
	return;
}

/*
	One blocking poll() call on all tracked fds: 
	pointer to array 1st el, count of els, 
	timeout -1 - wait indefinitely until any fd is ready; 
	returns count offds have events (or -1 - error).
*/
void Server::run() 
{
    m_running = true;

    while (m_running) 
	{
        int poll_count = poll(&m_poll_fds[0], m_poll_fds.size(), -1);
        
        if (poll_count < 0) 
		{
            if (errno == EINTR) continue;  // Signal interrupted
            throw std::runtime_error("poll() failed");
        }
        // Check all file descriptors POLLIN/OUT/ERR/HUP or revents - 0(client makes nothing)
        for (size_t i = 0; i < m_poll_fds.size(); ++i) 
		{
            if (m_poll_fds[i].revents == 0)
                continue;
            // Listener socket - new connection
            if (m_poll_fds[i].fd == m_listen_fd) 
			{
                if (m_poll_fds[i].revents & POLLIN) 
                    acceptClient();
            }
            // Client socket - read/write
            else 
			{
                int client_fd = m_poll_fds[i].fd;
                
                // Check for errors/hangup
                if (m_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) 
				{
                    disconnectClient(client_fd);
                    continue;
                }      
                // Ready to read
                if (m_poll_fds[i].revents & POLLIN) 
				{
                    if (!receiveData(client_fd)) 
					{
                        disconnectClient(client_fd);
                        continue;
                    }
                }
                // Ready to write (and has data to send)
                if (m_poll_fds[i].revents & POLLOUT) 
				{
                    Client* client = m_clients[client_fd].get();
                    if (!client->getOutBuf().empty())
                        sendData(client_fd);
                }
            }
        }
        cleanupDisconnectedClients();
    }
}

//  Method for graceful shutdown
void Server::stop()
{
	if (!m_running)
		return;
	m_running = false;
	// Broadcast NOTICE to all clients and attempt to flush their outbufs
	std::string shutdown_msg = ":ircserv NOTICE * :Server shutting down\r\n";
	// Collect fds first because disconnectClient() erases entries
	std::vector<int> fds;
	fds.reserve(m_clients.size());
	for (std::map<int, std::unique_ptr<Client>>::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
		fds.push_back(it->first);

	for (size_t i = 0; i < fds.size(); ++i)
	{
		int fd = fds[i];
		std::map<int, std::unique_ptr<Client>>::iterator it = m_clients.find(fd);
		if (it == m_clients.end())
			continue;
		it->second->appendToOutBuf(shutdown_msg);
		// Try to send immediately; sendData will handle partial sends and errors
		sendData(fd);
	}
	for (size_t i = 0; i < fds.size(); ++i)
	{
		disconnectClient(fds[i]);
	}
	if (m_listen_fd >= 0)
	{
		close(m_listen_fd);
		m_listen_fd = -1;
	}
	m_poll_fds.clear();
}

