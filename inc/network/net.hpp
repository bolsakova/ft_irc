/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   net.hpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/22 22:27:25 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/22 23:24:18 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef NET_HPP
#define NET_HPP

#include <string>
#include <vector>
#include <poll.h>         // poll, struct pollfd

//были все static в Server.cpp, на финише решить оставить в отдельном файле если больше нигде не использовать, или вернуть в Server и сделать статик 
int set_non_blocking(int fd);
int parse_port_strict(const std::string &port_str);
void disable_pollevent(std::vector<pollfd> &poll_fds, int fd, short flag);

#endif