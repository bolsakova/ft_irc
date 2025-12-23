/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 16:08:00 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/23 16:28:08 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/network/Channel.hpp"
#include "../../inc/network/Client.hpp"

// Конструктор по умолчанию: создаем пустой канал с выключенными режимами.
// Имя/топик/ключ пусты, списки участников/операторов/приглашенных — пустые.
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

// Устанавливаем топик канала: хранится строка, которую видят участники.
void Channel::setTopic(const std::string& topic){m_topic = topic;}

// Ставим ключ (пароль) для входа в канал при режиме +k.
void Channel::setKey(const std::string& key){m_key = key;}

// Возвращаем имя канала, чтобы сервер мог идентифицировать объект при маршрутизации.
const std::string& Channel::getName() const{return m_name;}

// Читаем текущий топик канала для ответов клиентам.
const std::string& Channel::getTopic() const{return m_topic;}

// Получаем текущий ключ канала, чтобы проверять JOIN.
const std::string& Channel::getKey() const{return m_key;}

// Добавляем участника: берем его fd из объекта Client и сохраняем в мапу.
// Если такой fd уже есть, мы просто обновляем указатель (на случай реконнекта).
void Channel::addMember(Client* client)
{
	if (!client)
		return;
	int fd = client->getFD();
	m_members[fd] = client;
}

// Удаляем участника по его fd (используется при PART/QUIT).Параллельно снимаем операторские права, если были.
void Channel::removeMember(int fd)
{
	m_members.erase(fd);
	m_operators.erase(fd);
}

// Проверяем, состоит ли fd в списке участников.
bool Channel::isMember(int fd) const{return m_members.find(fd) != m_members.end();}

// Отдаем весь список участников (fd -> Client*) для итерации или рассылки.
const std::map<int, Client*>& Channel::getMembers() const{return m_members;}

// Проверяем, пустой ли канал (нет участников).
bool Channel::isEmpty() const{return m_members.empty();}
