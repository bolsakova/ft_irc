/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 15:57:49 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/23 16:14:33 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <map>
#include <set>

class Client;

class Channel 
{
private:
    std::string m_name;                        // Имя канала (#general, #random)
    std::string m_topic;                       // Топик канала
    std::string m_key;                         // Пароль (если установлен mode +k)
    std::map<int, Client*> m_members;          // Все участники канала (fd -> Client*)
    std::set<int> m_operators;                 // Операторы канала (fd)
    std::set<int> m_invited;                   // Приглашенные пользователи (fd)
    
    // Режимы канала
    bool m_invite_only;                        // +i (только по приглашению)
    bool m_topic_protected;                    // +t (только ops меняют топик)
    int m_user_limit;                          // +l (лимит пользователей, 0 = нет лимита)

public:
    //Default constructor initializes empty channel metadata and disabled modes.
    Channel();
    Channel(const Channel& src);
    Channel& operator=(const Channel& rhs);
    ~Channel();
};

#endif // CHANNEL_HPP
