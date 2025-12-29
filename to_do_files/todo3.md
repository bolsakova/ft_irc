# План завершения проекта ft_irc

## 1. Завершение основной функциональности

### 1.1 Main функция (server.cpp)

**Теория:**
- Main должен принимать 2 аргумента: `<port> <password>`
- Создать экземпляр Server с портом и паролем
- Запустить бесконечный цикл `Server::run()`
- Обработать сигналы (SIGINT, SIGTERM) для graceful shutdown

**Реализация:**
```cpp
// main.cpp или server_main.cpp
#include "inc/network/Server.hpp"
#include <csignal>
#include <cstdlib>

// Global server pointer for signal handler
Server* g_server = NULL;

void signalHandler(int signum) {
    if (g_server) {
        std::cout << "\nShutting down server...\n";
        g_server->stop();
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <port> <password>\n";
        return 1;
    }

    // Validate port
    int port = std::atoi(argv[1]);
    if (port < 1024 || port > 65535) {
        std::cerr << "Error: Port must be between 1024 and 65535\n";
        return 1;
    }

    std::string password = argv[2];
    if (password.empty()) {
        std::cerr << "Error: Password cannot be empty\n";
        return 1;
    }

    try {
        Server server(port, password);
        g_server = &server;

        // Setup signal handlers
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        std::cout << "IRC Server starting on port " << port << "\n";
        server.run();  // Infinite loop with poll()
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
```

**Makefile обновление:**
```makefile
NAME = ircserv

SRCS = src/network/Server.cpp \
       src/network/Client.cpp \
       src/protocol/CommandHandler.cpp \
       src/protocol/Parser.cpp \
       src/protocol/MessageBuilder.cpp \
       main.cpp

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all
```

### 1.2 Server::run() - главный цикл

**Теория:**
- Один вызов `poll()` для всех file descriptors
- `poll()` вызывается ПЕРЕД каждым accept/read/send
- Использовать `fcntl(fd, F_SETFL, O_NONBLOCK)` для non-blocking режима
- НЕ использовать errno == EAGAIN для повторного чтения

**Реализация в Server.cpp:**
```cpp
void Server::run() {
    m_running = true;
    
    while (m_running) {
        // ЕДИНСТВЕННЫЙ вызов poll() во всей программе
        int poll_count = poll(&m_poll_fds[0], m_poll_fds.size(), -1);
        
        if (poll_count < 0) {
            if (errno == EINTR) continue;  // Signal interrupted
            throw std::runtime_error("poll() failed");
        }

        // Check all file descriptors
        for (size_t i = 0; i < m_poll_fds.size(); ++i) {
            if (m_poll_fds[i].revents == 0)
                continue;

            // Listener socket - new connection
            if (m_poll_fds[i].fd == m_listener_fd) {
                if (m_poll_fds[i].revents & POLLIN) {
                    acceptNewClient();
                }
            }
            // Client socket - read/write
            else {
                int client_fd = m_poll_fds[i].fd;
                
                // Check for errors/hangup
                if (m_poll_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    disconnectClient(client_fd);
                    continue;
                }
                
                // Ready to read
                if (m_poll_fds[i].revents & POLLIN) {
                    if (!receiveData(client_fd)) {
                        disconnectClient(client_fd);
                        continue;
                    }
                }
                
                // Ready to write (and has data to send)
                if (m_poll_fds[i].revents & POLLOUT) {
                    Client* client = m_clients[client_fd].get();
                    if (!client->getOutBuf().empty()) {
                        sendData(client_fd);
                    }
                }
            }
        }
        
        // Disconnect clients marked for removal
        cleanupDisconnectedClients();
    }
}
```

### 1.3 Что еще нужно добавить

**В Server.cpp:**
```cpp
// 1. Метод для graceful shutdown
void Server::stop() {
    m_running = false;
    
    // Broadcast QUIT to all clients
    std::string shutdown_msg = ":ircserv NOTICE * :Server shutting down\r\n";
    for (auto& pair : m_clients) {
        pair.second->appendToOutBuf(shutdown_msg);
        sendData(pair.first);
    }
    
    // Close all connections
    for (auto& pair : m_clients) {
        close(pair.first);
    }
    m_clients.clear();
    
    // Close listener
    if (m_listener_fd != -1) {
        close(m_listener_fd);
    }
}

// 2. Partial command handling
bool Server::receiveData(int fd) {
    char buffer[512];
    ssize_t bytes = recv(fd, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes <= 0) {
        return false;  // Connection closed or error
    }
    
    buffer[bytes] = '\0';
    Client* client = m_clients[fd].get();
    
    // Append to client's input buffer
    client->appendToInBuf(std::string(buffer, bytes));
    
    // Process complete commands (ending with \r\n)
    std::string& inbuf = client->getInBuf();
    size_t pos;
    
    while ((pos = inbuf.find("\r\n")) != std::string::npos) {
        std::string command = inbuf.substr(0, pos + 2);
        inbuf.erase(0, pos + 2);
        
        m_command_handler->handleCommand(command, *client);
    }
    
    // Check buffer overflow (max 512 bytes per IRC message)
    if (inbuf.size() > 512) {
        inbuf.clear();
        return false;
    }
    
    return true;
}
```

**Добавить в Client.hpp/cpp:**
```cpp
class Client {
private:
    std::string m_input_buffer;   // Partial commands
    std::string m_output_buffer;  // Data to send
    
public:
    void appendToInBuf(const std::string& data) { m_input_buffer += data; }
    std::string& getInBuf() { return m_input_buffer; }
    void clearInBuf() { m_input_buffer.clear(); }
};
```

---

## 2. Поведение программы и тестирование

### 2.1 Как должна работать программа

**Запуск:**
```bash
./ircserv 6667 mypassword
```

**Ожидаемое поведение:**
1. ✅ Сервер слушает на порту 6667
2. ✅ Принимает множественные соединения одновременно
3. ✅ НЕ блокируется - отвечает на все запросы параллельно
4. ✅ Обрабатывает частичные команды (partial commands)
5. ✅ Не падает при отключении клиента
6. ✅ Не зависает при флуде сообщений

### 2.2 Тестирование с netcat (nc)

**Тест 1: Базовое подключение**
```bash
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice User
# Ожидается: RPL_WELCOME (001-004)

PING :test
# Ожидается: PONG ircserv :test

QUIT :Bye
# Ожидается: отключение
```

**Тест 2: Частичные команды**
```bash
nc localhost 6667
PASS myp          # Partial - ничего не происходит
assword\r\n       # Complete - теперь обработается
NICK ali
ce\r\n            # Должно работать
```

**Тест 3: Множественные соединения**
```bash
# Terminal 1
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice

# Terminal 2 (параллельно!)
nc localhost 6667
PASS mypassword
NICK bob
USER bob 0 * :Bob

# Оба должны работать одновременно
```

**Тест 4: Неожиданное отключение**
```bash
nc localhost 6667
PASS mypassword
NICK alice
# Ctrl+C - kill nc
# Сервер должен продолжать работу
```

**Тест 5: Suspended client (flood test)**
```bash
# Terminal 1
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice
JOIN #test
# Ctrl+Z - suspend

# Terminal 2
nc localhost 6667
PASS mypassword
NICK bob
USER bob 0 * :Bob
JOIN #test
# Flood channel
for i in {1..1000}; do echo "PRIVMSG #test :Message $i"; done

# Terminal 1
fg  # Resume - все сообщения должны быть доставлены
```

### 2.3 Тестирование с IRC клиентом

**irssi:**
```bash
irssi
/connect localhost 6667 mypassword
/nick alice
/join #general
/msg bob Hello!
/topic #general New topic
/mode #general +i
/invite bob #general
/kick bob Bad behavior
```

**HexChat / WeeChat:**
```
/server add LocalIRC localhost/6667
/set irc.server.LocalIRC.password mypassword
/connect LocalIRC
/join #test
```

**Тест каналов:**
```
# User 1 (оператор)
JOIN #chan
MODE #chan +i        # Invite-only
MODE #chan +k secret # Set key
MODE #chan +l 5      # User limit

# User 2 (обычный)
JOIN #chan           # Fail: ERR_INVITEONLYCHAN

# User 1
INVITE user2 #chan

# User 2
JOIN #chan           # Success
```

### 2.4 Чеклист тестирования

- [ ] Запуск с правильными/неправильными аргументами
- [ ] Подключение с nc
- [ ] Подключение с IRC клиентом (irssi/hexchat)
- [ ] Аутентификация (PASS, NICK, USER)
- [ ] PING/PONG keepalive
- [ ] JOIN/PART каналов
- [ ] PRIVMSG (user-to-user, channel)
- [ ] KICK (operator only)
- [ ] INVITE (operator only on +i)
- [ ] TOPIC (view/set, +t mode)
- [ ] MODE (view, +i, +t, +k, +o, +l)
- [ ] Множественные клиенты одновременно
- [ ] Частичные команды
- [ ] Неожиданное отключение (Ctrl+C)
- [ ] Suspended client (Ctrl+Z + flood)
- [ ] Memory leaks (valgrind)

---

## 3. Рефакторинг

### 3.1 Структура проекта

**Текущая:**
```
ft_irc/
├── inc/
│   ├── network/
│   │   ├── Server.hpp
│   │   ├── Client.hpp
│   │   └── Channel.hpp
│   └── protocol/
│       ├── CommandHandler.hpp
│       ├── Parser.hpp
│       ├── MessageBuilder.hpp
│       └── Replies.hpp
├── src/
│   ├── network/
│   │   ├── Server.cpp
│   │   ├── Client.cpp
│   │   └── Channel.cpp
│   └── protocol/
│       ├── CommandHandler.cpp
│       ├── Parser.cpp
│       └── MessageBuilder.cpp
├── main.cpp
├── Makefile
└── README.md
```

**Предложение:** Оставить как есть - структура логичная и читаемая.

### 3.2 Комментарии - упрощение

**Текущий стиль (verbose):**
```cpp
/**
 * @brief Handle PING command - respond with PONG to keep connection alive.
 * Format: PING <token> or PING :<token>
 * 
 * @param client Client sending the command
 * @param msg Parsed message with command and parameters
 * 
 * Algorithm:
 * 			1. Check if client is registered -> error 451
 * 			2. Extract token from params[0] or trailing
 * 			3. If token is empty -> use server_name as default
 * 			4. Build PONG response: :<server> PONG <server> :<token>
 * 			5. Send PONG to client
 */
void CommandHandler::handlePing(Client& client, const Message& msg);
```

**Предложенный стиль (clean):**
```cpp
/**
 * @brief Respond to PING with PONG to keep connection alive
 * @param client Client sending PING
 * @param msg Parsed message (format: PING <token>)
 */
void CommandHandler::handlePing(Client& client, const Message& msg);
```

**Заголовок файла (достаточно одного блока):**
```cpp
/**
 * CommandHandler.cpp
 * 
 * IRC command dispatcher and handler implementation.
 * Routes parsed messages to appropriate command handlers (PASS, NICK, USER,
 * PING, QUIT, PRIVMSG, JOIN, PART, KICK, INVITE, TOPIC, MODE).
 * 
 * RFC 1459: https://tools.ietf.org/html/rfc1459
 */
```

### 3.3 Рефакторинг - TODO list

**1. Убрать избыточные комментарии:**
- ✅ Оставить краткие @brief в заголовках
- ✅ Удалить "Algorithm: 1. 2. 3..." (код сам себя документирует)
- ✅ Добавить файловые заголовки вместо повторяющихся блоков

**2. Единый стиль:**
```cpp
// ✅ GOOD - consistent naming
void handleCommand(const std::string& cmd, Client& client);
bool isValidNickname(const std::string& nick);
Channel* findChannel(const std::string& name);

// ❌ BAD - inconsistent
void handle_command(...);  // snake_case
bool IsValidNickname(...);  // PascalCase
```

**3. Magic numbers → constants:**
```cpp
// ❌ BEFORE
if (nickname.length() > 9) ...
if (channel.length() > 50) ...

// ✅ AFTER
const int MAX_NICKNAME_LEN = 9;
const int MAX_CHANNEL_LEN = 50;
const int IRC_MESSAGE_MAX = 512;

if (nickname.length() > MAX_NICKNAME_LEN) ...
```

**4. Error handling consolidation:**
```cpp
// Helper method
void CommandHandler::sendError(Client& client, int code, 
                               const std::string& target, 
                               const std::string& message) {
    std::string error = MessageBuilder::buildErrorReply(
        m_server_name, code, client.getNickname(), target, message
    );
    sendReply(client, error);
}

// Usage
if (!client.isRegistered()) {
    sendError(client, ERR_NOTREGISTERED, "", "You have not registered");
    return;
}
```

### 3.4 README.md - обновление

```markdown
# ft_irc - IRC Server Implementation

IRC server compliant with RFC 1459, written in C++98.

## Features

- Multi-client support with non-blocking I/O (poll)
- User authentication and registration
- Channel management (create, join, part)
- Channel modes: +i (invite-only), +t (topic protection), +k (key), +o (operator), +l (limit)
- Private messaging (user-to-user and channels)
- Operator commands: KICK, INVITE, TOPIC, MODE

## Building

```bash
make
```

## Usage

```bash
./ircserv <port> <password>
```

Example:
```bash
./ircserv 6667 mypassword
```

## Connecting

**With netcat:**
```bash
nc localhost 6667
PASS mypassword
NICK alice
USER alice 0 * :Alice User
```

**With IRC client (irssi):**
```bash
irssi
/connect localhost 6667 mypassword
/nick alice
/join #general
```

## Implemented Commands

### Registration
- `PASS <password>` - Authenticate with server
- `NICK <nickname>` - Set nickname
- `USER <username> 0 * :<realname>` - Set username

### Connection
- `PING <token>` - Keepalive
- `QUIT [:<reason>]` - Disconnect

### Messaging
- `PRIVMSG <target> :<message>` - Send message to user or channel

### Channels
- `JOIN <channel> [key]` - Join or create channel
- `PART <channel> [:<reason>]` - Leave channel

### Operator Commands
- `KICK <channel> <user> [:<reason>]` - Remove user from channel
- `INVITE <user> <channel>` - Invite user to channel
- `TOPIC <channel> [:<new topic>]` - View or set channel topic
- `MODE <channel> [+/-modes] [parameters]` - View or change channel modes

## Channel Modes

- `+i` / `-i` - Invite-only
- `+t` / `-t` - Topic protection (only operators can change)
- `+k <key>` / `-k` - Channel key/password
- `+o <nick>` / `-o <nick>` - Grant/revoke operator privileges
- `+l <limit>` / `-l` - User limit

## Testing

Run unit tests:
```bash
make test
./test_commands
```

## Project Structure

```
inc/network/     - Server, Client, Channel classes
inc/protocol/    - Command handler, parser, message builder
src/            - Implementation files
main.cpp        - Entry point
```

## Authors

- Tanja (Protocol layer)
- Alima (Network layer)
```

---

## 4. Подготовка к защите

### 4.1 Теоретические вопросы

**1. Что такое IRC?**
> Internet Relay Chat - протокол для текстового общения в реальном времени, разработан в 1988 году, описан в RFC 1459.

**2. Почему используется poll(), а не select()?**
> - poll() не ограничен FD_SETSIZE (обычно 1024)
> - Более чистый API (массив структур pollfd)
> - Легче масштабируется на большое количество соединений

**3. Что такое non-blocking I/O?**
> Режим, при котором операции read/write/accept не блокируют выполнение программы. Если данных нет - возвращается EAGAIN/EWOULDBLOCK.

**4. Почему ТОЛЬКО ОДИН poll()?**
> - Избежать race conditions
> - Централизованное управление всеми соединениями
> - Предсказуемое поведение
> - Требование subject

**5. Зачем fcntl(fd, F_SETFL, O_NONBLOCK)?**
> Переводит socket в неблокирующий режим. Операции возвращают управление немедленно, даже если не могут завершиться.

**6. Как обрабатываются частичные команды?**
> Накапливаются в input buffer клиента до получения \r\n. Только полные команды передаются в CommandHandler.

**7. Что происходит при отключении клиента?**
> - poll() возвращает POLLHUP/POLLERR
> - Рассылается QUIT всем каналам
> - Удаляется из всех каналов
> - Закрывается socket
> - Удаляется из m_clients

**8. Разница между оператором и обычным пользователем?**
> - Operator: может KICK, INVITE (на +i), менять TOPIC (на +t), MODE
> - User: может только JOIN, PART, PRIVMSG

**9. Что такое режим +i?**
> Invite-only: войти может только приглашенный оператором пользователь.

**10. Как работает режим +k?**
> Channel key: для входа нужно указать пароль - JOIN #chan password

### 4.2 Навигация по коду - быстрые ответы

**"Покажите, где poll()"**
→ `src/network/Server.cpp`, метод `Server::run()`, строка ~50

**"Где обрабатывается NICK?"**
→ `src/protocol/CommandHandler.cpp`, метод `handleNick()`, строка ~260

**"Как проверяется, что пользователь оператор?"**
→ `Channel::isOperator(int fd)` + проверка в командах:
```cpp
if (!chan->isOperator(client.getFD())) {
    sendError(..., ERR_CHANOPRIVSNEEDED);
}
```

**"Где хранятся клиенты?"**
→ `Server::m_clients` - `std::map<int, std::unique_ptr<Client>>`

**"Где хранятся каналы?"**
→ `Server::m_channels` - `std::map<std::string, std::unique_ptr<Channel>>`

**"Как обрабатывается режим +t?"**
→ `CommandHandler::handleTopic()`:
```cpp
if (chan->isTopicProtected() && !chan->isOperator(client.getFD())) {
    return ERR_CHANOPRIVSNEEDED;
}
```

### 4.3 Демонстрация работы

**Подготовьте 3 terminal:**

**Terminal 1 - Server:**
```bash
./ircserv 6667 pass
```

**Terminal 2 - Alice (operator):**
```bash
nc localhost 6667
PASS pass
NICK alice
USER alice 0 * :Alice
JOIN #test
MODE #test +i
TOPIC #test :Welcome to test channel
```

**Terminal 3 - Bob (user):**
```bash
nc localhost 6667
PASS pass
NICK bob
USER bob 0 * :Bob
JOIN #test         # FAIL - invite only
# Alice invites
# JOIN #test       # SUCCESS
PRIVMSG #test :Hello everyone!
TOPIC #test :New topic  # FAIL if +t
```

### 4.4 Возможные замечания evaluator

**1. "Почему нет поддержки SSL/TLS?"**
> Не требуется в subject. Это базовая реализация IRC без шифрования.

**2. "Почему не используете epoll (Linux) или kqueue (BSD)?"**
> poll() - переносимое решение, работает на всех UNIX системах. Для проекта достаточно.

**3. "Почему не C++11/14/17?"**
> Требование subject - C++98. Это учит работать без современных удобств (auto, lambda, etc).

**4. "Memory leaks?"**
> Используем std::unique_ptr для RAII. Можно проверить valgrind:
```bash
valgrind --leak-check=full ./ircserv 6667 pass
```

**5. "Что делать, если сервер зависнет?"**
> Не должен - poll() асинхронный, все операции non-blocking. Если зависает - баг.

### 4.5 Чеклист перед защитой

- [ ] Код компилируется без warnings
- [ ] Makefile имеет все правила (all, clean, fclean, re)
- [ ] Программа принимает ровно 2 аргумента
- [ ] ТОЛЬКО ОДИН poll() во всем коде
- [ ] Все fcntl() используют F_SETFL, O_NONBLOCK
- [ ] Нет использования errno == EAGAIN для retry
- [ ] Сервер слушает на указанном порту
- [ ] nc успешно подключается
- [ ] IRC клиент успешно подключается
- [ ] Множественные подключения работают параллельно
- [ ] Частичные команды обрабатываются корректно
- [ ] Убитый клиент не ломает сервер
- [ ] Suspended client + flood не виснет
- [ ] Все команды работают (PASS, NICK, USER, PING, JOIN, PART, PRIVMSG, KICK, INVITE, TOPIC, MODE)
- [ ] Operator privileges работают корректно
- [ ] README.md заполнен
- [ ] Нет memory leaks (проверено valgrind)

---

## Распределение задач (Tanja + Alima)

### Tanja (Protocol layer)
- [x] Все команды реализованы
- [ ] Рефакторинг комментариев в CommandHandler.cpp
- [ ] Написать README.md
- [ ] Подготовить теоретические ответы
- [ ] Протестировать все команды с irssi

### Alima (Network layer)
- [ ] Дописать main.cpp
- [ ] Дописать Server::run() с poll()
- [ ] Реализовать partial command handling
- [ ] Реализовать graceful shutdown
- [ ] Тестирование с nc (partial, disconnect, flood)
- [ ] Проверка valgrind на memory leaks

### Вместе
- [ ] Финальное тестирование всех сценариев
- [ ] Прогон по evaluation sheet
- [ ] Взаимная подготовка к вопросам
- [ ] Dry run защиты

---

