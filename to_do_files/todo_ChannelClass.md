Отлично! Вот подробное объяснение для вашей напарницы.

---

## 📖 Зачем нужен класс Channel?

### Что такое Channel (канал) в IRC?

**Channel** - это "комната" для общения, где несколько пользователей могут обмениваться сообщениями одновременно.

**Аналогия:**
- Client = человек
- Channel = комната для разговора
- Server = здание со множеством комнат

**Примеры каналов:**
- `#general` - общий канал для всех
- `#random` - канал для случайных обсуждений
- `#help` - канал для помощи

---

## 🎯 Что должен хранить класс Channel?

### Основные данные:

```cpp
class Channel {
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
};
```

---

## 🔧 Какие методы нужны?

### 1. Управление участниками

```cpp
// Добавить пользователя в канал
void addMember(Client* client);

// Удалить пользователя из канала
void removeMember(int fd);

// Проверить, состоит ли пользователь на канале
bool isMember(int fd) const;

// Получить всех участников
const std::map<int, Client*>& getMembers() const;

// Проверить, пустой ли канал
bool isEmpty() const;
```

### 2. Управление операторами

```cpp
// Сделать пользователя оператором
void addOperator(int fd);

// Забрать права оператора
void removeOperator(int fd);

// Проверить, является ли пользователь оператором
bool isOperator(int fd) const;
```

### 3. Управление режимами

```cpp
// Установить/снять режим invite-only (+i/-i)
void setInviteOnly(bool enable);
bool isInviteOnly() const;

// Установить/снять защиту топика (+t/-t)
void setTopicProtected(bool enable);
bool isTopicProtected() const;

// Установить/удалить ключ (+k/-k)
void setKey(const std::string& key);
void removeKey();
bool hasKey() const;
const std::string& getKey() const;

// Установить/снять лимит пользователей (+l/-l)
void setUserLimit(int limit);
int getUserLimit() const;
```

### 4. Управление топиком

```cpp
// Установить топик канала
void setTopic(const std::string& topic);

// Получить топик канала
const std::string& getTopic() const;

// Проверить, установлен ли топик
bool hasTopic() const;
```

### 5. Управление приглашениями

```cpp
// Пригласить пользователя (для режима +i)
void addInvited(int fd);

// Проверить, приглашен ли пользователь
bool isInvited(int fd) const;

// Убрать из списка приглашенных (после входа)
void removeInvited(int fd);
```

### 6. Утилиты

```cpp
// Получить имя канала
const std::string& getName() const;

// Отправить сообщение всем участникам
void broadcast(const std::string& message, int exclude_fd = -1);
```

---

## 📝 Как CommandHandler будет использовать Channel?

### Пример 1: JOIN команда

```cpp
// В CommandHandler::handleJoin()
void CommandHandler::handleJoin(Client& client, const Message& msg) {
    std::string channel_name = msg.params[0];
    
    // Найти или создать канал
    Channel* chan = m_server.findChannel(channel_name);
    if (!chan) {
        chan = m_server.createChannel(channel_name);
        chan->addOperator(client.getFD()); // Первый участник = оператор
    }
    
    // Проверить режимы канала
    if (chan->isInviteOnly() && !chan->isInvited(client.getFD())) {
        // Ошибка: нужно приглашение
        return;
    }
    
    if (chan->hasKey()) {
        std::string provided_key = msg.params[1];
        if (provided_key != chan->getKey()) {
            // Ошибка: неверный ключ
            return;
        }
    }
    
    // Добавить клиента в канал
    chan->addMember(&client);
    
    // Уведомить всех участников
    std::string join_msg = ":" + client.getNickname() + 
                          "!user@host JOIN :" + channel_name + "\r\n";
    chan->broadcast(join_msg);
}
```

### Пример 2: PRIVMSG на канал

```cpp
// В CommandHandler::handlePrivmsg()
void CommandHandler::handlePrivmsg(Client& client, const Message& msg) {
    std::string target = msg.params[0];
    std::string message = msg.trailing;
    
    // Если target начинается с # - это канал
    if (target[0] == '#') {
        Channel* chan = m_server.findChannel(target);
        if (!chan) {
            // Ошибка: канал не существует
            return;
        }
        
        if (!chan->isMember(client.getFD())) {
            // Ошибка: не состоишь на канале
            return;
        }
        
        // Отправить сообщение всем, кроме отправителя
        std::string msg_to_send = ":" + client.getNickname() + 
                                  "!user@host PRIVMSG " + target + 
                                  " :" + message + "\r\n";
        chan->broadcast(msg_to_send, client.getFD()); // exclude отправителя
    }
}
```

### Пример 3: PART (выход из канала)

```cpp
void CommandHandler::handlePart(Client& client, const Message& msg) {
    std::string channel_name = msg.params[0];
    
    Channel* chan = m_server.findChannel(channel_name);
    if (!chan || !chan->isMember(client.getFD())) {
        // Ошибка
        return;
    }
    
    // Уведомить всех участников
    std::string part_msg = ":" + client.getNickname() + 
                          "!user@host PART " + channel_name + "\r\n";
    chan->broadcast(part_msg);
    
    // Удалить из канала
    chan->removeMember(client.getFD());
    
    // Если канал пустой - удалить
    if (chan->isEmpty()) {
        m_server.removeChannel(channel_name);
    }
}
```

### Пример 4: MODE (изменение режимов)

```cpp
void CommandHandler::handleMode(Client& client, const Message& msg) {
    std::string channel_name = msg.params[0];
    std::string mode_string = msg.params[1];
    
    Channel* chan = m_server.findChannel(channel_name);
    
    // Проверить права оператора
    if (!chan->isOperator(client.getFD())) {
        // Ошибка: нет прав
        return;
    }
    
    // Парсить режимы
    if (mode_string == "+i")
        chan->setInviteOnly(true);
    else if (mode_string == "-i")
        chan->setInviteOnly(false);
    else if (mode_string == "+t")
        chan->setTopicProtected(true);
    // и т.д.
}
```

---

## 🗂️ Где хранить каналы?

### В классе Server нужно добавить:

```cpp
class Server {
private:
    std::map<int, std::unique_ptr<Client>> m_clients;
    std::map<std::string, std::unique_ptr<Channel>> m_channels; // НОВОЕ!
    
public:
    // Методы для работы с каналами
    Channel* findChannel(const std::string& name);
    Channel* createChannel(const std::string& name);
    void removeChannel(const std::string& name);
    const std::map<std::string, std::unique_ptr<Channel>>& getChannels() const;
};
```

---

## 📋 Резюме для напарницы

**Что нужно создать:**

1. **Класс Channel** (`inc/network/Channel.hpp`, `src/network/Channel.cpp`)
   - Хранит: имя, топик, участников, операторов, режимы
   - Методы: управление участниками, режимами, топиком

2. **Дополнить Server**:
   - Добавить `std::map<std::string, std::unique_ptr<Channel>> m_channels`
   - Методы: `findChannel()`, `createChannel()`, `removeChannel()`, `getChannels()`

3. **Дополнить Client** (если еще не сделано):
   - Добавить IRC-поля: nickname, username, realname, authenticated, registered
   - Getters/Setters для них

**Какие команды требуют Channel:**
- JOIN (вход на канал)
- PART (выход из канала)
- PRIVMSG #channel (сообщение на канал)
- KICK (выгнать пользователя)
- INVITE (пригласить)
- TOPIC (установить/посмотреть топик)
- MODE (изменить режимы)

---

Передайте это напарнице! После того как она реализует Channel, вы сможете доделать все оставшиеся команды. А пока можем сделать команды которые не требуют каналов (QUIT, PRIVMSG только для личных сообщений).
