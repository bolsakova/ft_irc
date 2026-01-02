/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 16:08:00 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/23 17:00:55 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../inc/network/Channel.hpp"
#include "../../inc/network/Client.hpp"
#include <iostream>

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

// Проверяем, задан ли топик (не пустая строка).
bool Channel::hasTopic() const{return !m_topic.empty();}

// Возвращаем имя канала, чтобы сервер мог идентифицировать объект при маршрутизации.
const std::string& Channel::getName() const{return m_name;}

// Читаем текущий топик канала для ответов клиентам.
const std::string& Channel::getTopic() const{return m_topic;}

// Добавляем участника: берем его fd из объекта Client и сохраняем в мапу.
// Если такой fd уже есть, мы просто обновляем указатель (на случай реконнекта).
void Channel::addMember(Client* client)
{
	if (!client)
		return;
	m_members[client->getFD()] = client;
}

// Удаляем участника по его fd (используется при PART/QUIT). Параллельно снимаем операторские права, если были.
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

// Делаем пользователя оператором: добавляем его fd в множество операторов.
void Channel::addOperator(int fd){m_operators.insert(fd);}

// Забираем права оператора у пользователя с указанным fd.
void Channel::removeOperator(int fd){m_operators.erase(fd);}

// Проверяем, является ли пользователь оператором канала.
bool Channel::isOperator(int fd) const{return m_operators.find(fd) != m_operators.end();}

// Устанавливаем лимит пользователей +l (0 или меньше = нет лимита).
void Channel::setUserLimit(int limit){m_user_limit = limit;}

// Включаем/выключаем режим +i (invite-only).
void Channel::setInviteOnly(bool enable){m_invite_only = enable;}

// Включаем/выключаем защиту топика +t (только операторы могут менять).
void Channel::setTopicProtected(bool enable){m_topic_protected = enable;}

// Устанавливаем ключ (пароль) на канал для режима +k.
void Channel::setKey(const std::string& key){m_key = key;}

// Снимаем ключ канала (эквивалент -k).
void Channel::removeKey(){m_key.clear();}

// Проверяем, включен ли режим +i.
bool Channel::isInviteOnly() const{return m_invite_only;}

// Проверяем, включен ли режим +t.
bool Channel::isTopicProtected() const{return m_topic_protected;}

// Проверяем, установлен ли ключ (непустая строка).
bool Channel::hasKey() const{return !m_key.empty();}

// Получаем текущий ключ канала.
const std::string& Channel::getKey() const{return m_key;}

// Получаем текущий лимит пользователей.
int Channel::getUserLimit() const{return m_user_limit;}

// Добавляем пользователя в список приглашенных (для режима +i).
void Channel::addInvited(int fd){m_invited.insert(fd);}

// Проверяем, есть ли у пользователя приглашение.
bool Channel::isInvited(int fd) const{return m_invited.find(fd) != m_invited.end();}

// Убираем пользователя из списка приглашенных (после входа или revoke).
void Channel::removeInvited(int fd){m_invited.erase(fd);}

// Рассылаем сообщение всем участникам, опционально исключая отправителя по fd.
void Channel::broadcast(const std::string& message, int exclude_fd)
{
	std::cout << "Broadcasting to " << m_members.size() << " members" << std::endl;
    for (std::map<int, Client*>::iterator it = m_members.begin();
         it != m_members.end(); ++it)
    {
        if (it->first == exclude_fd) {
            std::cout << "  Skipping fd " << it->first << std::endl;
            continue;
        }
        
        std::cout << "  Sending to fd " << it->first << " (" 
                  << it->second->getNickname() << ")" << std::endl;
        it->second->appendToOutBuf(message);
    }
}
