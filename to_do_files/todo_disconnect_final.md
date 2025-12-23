Отлично! Вариант 3 - это правильный архитектурный подход. Вот подробная инструкция для вашей напарницы.

---

# 📋 Инструкция для напарницы (сетевик)

## Часть 1: Добавить IRC-специфичные поля в Client

Это поля для протокольного уровня, но хранятся в Client.

### **Изменения в Client.hpp:**

```cpp
class Client
{
	private:
		int			m_fd;
		std::string	m_inbuf;
		std::string	m_outbuf;
		bool		m_peer_closed;
		
		// ============ IRC protocol fields (NEW!) ============
		std::string m_nickname;         // IRC nickname
		std::string m_username;         // IRC username
		std::string m_realname;         // IRC realname
		bool        m_authenticated;    // Passed PASS command?
		bool        m_registered;       // Completed registration (PASS+NICK+USER)?
		
		// ============ Disconnect management (NEW!) ============
		bool        m_should_disconnect; // Should server disconnect this client?
		std::string m_quit_reason;       // Reason for disconnection (for QUIT)
	
	public:
		// ... existing methods ...
		
		// ============ IRC protocol getters/setters (NEW!) ============
		
		/**
		 * @brief Get client's IRC nickname
		 * @return Current nickname (empty if not set)
		 */
		const std::string& getNickname() const;
		
		/**
		 * @brief Set client's IRC nickname
		 * @param nick New nickname
		 */
		void setNickname(const std::string& nick);
		
		/**
		 * @brief Get client's IRC username
		 * @return Current username (empty if not set)
		 */
		const std::string& getUsername() const;
		
		/**
		 * @brief Set client's IRC username
		 * @param user New username
		 */
		void setUsername(const std::string& user);
		
		/**
		 * @brief Get client's IRC realname
		 * @return Current realname (empty if not set)
		 */
		const std::string& getRealname() const;
		
		/**
		 * @brief Set client's IRC realname
		 * @param real New realname
		 */
		void setRealname(const std::string& real);
		
		/**
		 * @brief Check if client passed PASS authentication
		 * @return True if authenticated, false otherwise
		 */
		bool isAuthenticated() const;
		
		/**
		 * @brief Set authentication status
		 * @param auth Authentication status
		 */
		void setAuthenticated(bool auth);
		
		/**
		 * @brief Check if client completed full registration (PASS+NICK+USER)
		 * @return True if registered, false otherwise
		 */
		bool isRegistered() const;
		
		/**
		 * @brief Set registration status
		 * @param reg Registration status
		 */
		void setRegistered(bool reg);
		
		// ============ Disconnect management methods (NEW!) ============
		
		/**
		 * @brief Mark client for disconnection (for QUIT command)
		 * @param reason Reason for disconnection
		 * 
		 * @details
		 * This method marks client for later disconnection by server.
		 * Server will check shouldDisconnect() in main loop and
		 * properly disconnect the client after processing current command.
		 */
		void markForDisconnect(const std::string& reason);
		
		/**
		 * @brief Check if client should be disconnected
		 * @return True if client should be disconnected, false otherwise
		 */
		bool shouldDisconnect() const;
		
		/**
		 * @brief Get disconnection reason (for logging/notifications)
		 * @return Quit reason string
		 */
		const std::string& getQuitReason() const;
};
```

### **Изменения в Client.cpp:**

```cpp
// Обновить конструктор - добавить инициализацию новых полей:
Client::Client(int fd)
	: m_fd(fd), 
	  m_inbuf(""), 
	  m_outbuf(""), 
	  m_peer_closed(false),
	  m_nickname(""),              // NEW
	  m_username(""),              // NEW
	  m_realname(""),              // NEW
	  m_authenticated(false),      // NEW
	  m_registered(false),         // NEW
	  m_should_disconnect(false),  // NEW
	  m_quit_reason("")            // NEW
{}

// Добавить реализацию новых методов в конец файла:

// ============ IRC protocol getters/setters ============

const std::string& Client::getNickname() const {
	return m_nickname;
}

void Client::setNickname(const std::string& nick) {
	m_nickname = nick;
}

const std::string& Client::getUsername() const {
	return m_username;
}

void Client::setUsername(const std::string& user) {
	m_username = user;
}

const std::string& Client::getRealname() const {
	return m_realname;
}

void Client::setRealname(const std::string& real) {
	m_realname = real;
}

bool Client::isAuthenticated() const {
	return m_authenticated;
}

void Client::setAuthenticated(bool auth) {
	m_authenticated = auth;
}

bool Client::isRegistered() const {
	return m_registered;
}

void Client::setRegistered(bool reg) {
	m_registered = reg;
}

// ============ Disconnect management methods ============

void Client::markForDisconnect(const std::string& reason) {
	m_should_disconnect = true;
	m_quit_reason = reason;
}

bool Client::shouldDisconnect() const {
	return m_should_disconnect;
}

const std::string& Client::getQuitReason() const {
	return m_quit_reason;
}
```

---

## Часть 2: Добавить метод getClients() в Server

Протокольному слою нужен доступ к списку клиентов для поиска пользователей по nickname.

### **Изменения в Server.hpp:**

```cpp
class Server
{
	private:
		// ... existing private members ...
	
	public:
		// ... existing public methods ...
		
		/**
		 * @brief Get const reference to clients map
		 * @return Const reference to map of all connected clients
		 * 
		 * @details
		 * Used by protocol layer (CommandHandler) to:
		 * - Find users by nickname (for PRIVMSG)
		 * - Check if nickname is already in use (for NICK)
		 * - Iterate through users for broadcasting
		 */
		const std::map<int, std::unique_ptr<Client>>& getClients() const;
};
```

### **Изменения в Server.cpp:**

```cpp
// Добавить в конец файла перед run():

const std::map<int, std::unique_ptr<Client>>& Server::getClients() const {
	return m_clients;
}
```

---

## Часть 3: Проверять shouldDisconnect() в Server::run()

Server должен проверять флаг `shouldDisconnect()` после обработки команд и отключать клиентов.

### **Изменения в Server.cpp в методе `run()`:**

Найти место где вызывается обработка команд (где-то вызывается что-то вроде `processCommand()` или аналогичное).

После обработки всех команд добавить проверку:

```cpp
void Server::run()
{
	// ... existing code ...
	
	while(true)
	{
		// ... existing poll() code ...
		
		for (int i = static_cast<int>(m_poll_fds.size()) - 1; i >= 0; --i)
		{
			int fd = m_poll_fds[i].fd;
			short revents = m_poll_fds[i].revents;
			
			// ... existing event handling ...
			
			if (fd != m_listen_fd)
			{
				// ... existing POLLIN/POLLOUT handling ...
				
				// ============ NEW: Check if client should disconnect ============
				// After processing all events for this client, check disconnect flag
				auto it = m_clients.find(fd);
				if (it != m_clients.end() && it->second->shouldDisconnect())
				{
					std::cout << "Client fd " << fd << " marked for disconnect: " 
					          << it->second->getQuitReason() << "\n";
					disconnectClient(fd);
					continue; // Don't process further events for this fd
				}
				// ============ END NEW ============
			}
		}
	}
}
```

**Важно:** Эту проверку нужно добавить **после обработки POLLIN/POLLOUT**, но **до конца итерации** по дескрипторам. Точное место зависит от текущей структуры вашего кода.

---

## 📝 Резюме для напарницы

**Что нужно добавить:**

### 1. **Client.hpp** (8 новых полей + 15 новых методов):
- IRC поля: `m_nickname`, `m_username`, `m_realname`, `m_authenticated`, `m_registered`
- Disconnect поля: `m_should_disconnect`, `m_quit_reason`
- Getters/Setters для всех новых полей
- Методы: `markForDisconnect()`, `shouldDisconnect()`, `getQuitReason()`

### 2. **Client.cpp**:
- Обновить конструктор (инициализация новых полей)
- Реализовать все новые методы (~20 строк кода)

### 3. **Server.hpp** (1 новый метод):
- Публичный метод `getClients()`

### 4. **Server.cpp**:
- Реализация `getClients()` (1 строка)
- В `run()` добавить проверку `shouldDisconnect()` после обработки событий

**Зачем это нужно:**
- IRC поля нужны протокольщику для команд PASS, NICK, USER, PRIVMSG
- `markForDisconnect()` нужен для безопасного отключения (QUIT команда)
- `getClients()` нужен для поиска пользователей по nickname

**Время реализации:** ~15-20 минут

---

После того как напарница добавит эти изменения, я смогу реализовать QUIT и PRIVMSG команды! Передайте ей эту инструкцию?
