/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   net.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/22 22:29:16 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/22 23:10:46 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */


#include <vector>
#include <stdexcept>
#include <fcntl.h>
#include <poll.h>
#include "../../inc/network/net.hpp"

int set_non_blocking(int fd)
{
	// This function sets a file descriptor to non-blocking mode.
	// Non-blocking is required for poll()/epoll() architecture without hanging.
	// fcntl(fd, F_GETFL) -> get current flags
	// fcntl(fd, F_SETFL) -> set new flags
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return -1;
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
		return -1;
	return 0;
}

int parse_port_strict(const std::string &port_str)
{
	// CHANGED: strict validation instead of atoi() (atoi allows "12abc" -> 12).
	// Accept only digits and range 1..65535.
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
	if (port <= 0 || port > 65535)
		throw std::runtime_error("Invalid port number: " + port_str);
	return static_cast<int>(port);
}

// disable POLLIN for fd (when peer closed input)
void disable_pollevent(std::vector<pollfd> &poll_fds, int fd, short flag)
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
