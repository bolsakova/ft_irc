/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 16:08:00 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/23 16:10:55 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/network/Channel.hpp"

Channel::Channel()
	: m_name(),
	  m_topic(),
	  m_key(),
	  m_members(),
	  m_operators(),
	  m_invited(),
	  m_invite_only(false),
	  m_topic_protected(false),
	  m_user_limit(0)
{}

Channel::Channel(const Channel& src)
	: m_name(src.m_name),
	  m_topic(src.m_topic),
	  m_key(src.m_key),
	  m_members(src.m_members),
	  m_operators(src.m_operators),
	  m_invited(src.m_invited),
	  m_invite_only(src.m_invite_only),
	  m_topic_protected(src.m_topic_protected),
	  m_user_limit(src.m_user_limit)
{}

Channel& Channel::operator=(const Channel& rhs)
{
	if (this != &rhs)
	{
		m_name = rhs.m_name;
		m_topic = rhs.m_topic;
		m_key = rhs.m_key;
		m_members = rhs.m_members;
		m_operators = rhs.m_operators;
		m_invited = rhs.m_invited;
		m_invite_only = rhs.m_invite_only;
		m_topic_protected = rhs.m_topic_protected;
		m_user_limit = rhs.m_user_limit;
	}
	return *this;
}

Channel::~Channel() {}
