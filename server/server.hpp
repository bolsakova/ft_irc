/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:10 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/04 00:24:58 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <vector>
#include <unordered_map>

// temporal data structure to hold client information. TODO: move to a header file.
struct Client
{
	int id;
	std::string inbuf;
	std::string outbuf;
};

int create_and_bind(const char *port_str);
int set_non_blocking(int fd);
// void run_server(int listen_fd);
// void handle_new_connection(int listen_fd, std::unordered_map<int, Client> &clients, std::vector<struct pollfd> &poll_fds);
// void handle_client_data(int client_fd, std::unordered_map<int, Client> &clients, std::vector<struct pollfd> &poll_fds);
// void close_client_connection(int client_fd, std::unordered_map<int, Client> &clients, std::vector<struct pollfd> &poll_fds);

#endif // SERVER_HPP