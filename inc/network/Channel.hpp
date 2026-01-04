/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Channel.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/23 15:57:49 by aokhapki          #+#    #+#             */
/*   Updated: 2026/01/04 22:20:14 by aokhapki         ###   ########.fr       */
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
    std::string m_name;
    std::string m_topic;
    std::string m_key;                         // Password when mode +k is set
    std::map<int, Client*> m_members;          // All channel members (fd -> Client*)
    std::set<int> m_operators;                 // Channel operators (fd)
    std::set<int> m_invited;                   // Invited users (fd)
    
    // Channel modes
    bool m_invite_only;                        // +i (invite only)
    bool m_topic_protected;                    // +t (only ops can change topic)
    int m_user_limit;                          // +l (user limit, 0 means no limit)

public:
    // OCF
    Channel();
    Channel(const Channel& src);
    Channel& operator=(const Channel& rhs);
    ~Channel();

    // === Metadata ===
    void setTopic(const std::string& topic);
    const std::string& getName() const;
    const std::string& getTopic() const;
    bool hasTopic() const;

    // === Membership ===
    void addMember(Client* client);
    void removeMember(int fd);
    bool isMember(int fd) const;
    const std::map<int, Client*>& getMembers() const;
    bool isEmpty() const;

    // === Operators ===
    void addOperator(int fd);
    void removeOperator(int fd);
    bool isOperator(int fd) const;

    // === Modes ===
    void setUserLimit(int limit);
    void setInviteOnly(bool enable);
    void setTopicProtected(bool enable);
    void setKey(const std::string& key);
    void removeKey();
    int getUserLimit() const;
    const std::string& getKey() const;
    bool hasKey() const;
    bool isInviteOnly() const;
    bool isTopicProtected() const;

    // === Invites ===
    void addInvited(int fd);
    bool isInvited(int fd) const;
    void removeInvited(int fd);

    // === Utils ===
    void broadcast(const std::string& message, int exclude_fd = -1);
};

#endif
