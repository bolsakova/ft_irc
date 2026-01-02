## 🔧 ЭТАП 4: User MODE support

---

## Изменение 4.1 - Добавить поддержку user modes в Client

### В файле Client.hpp

Найти секцию с **private переменными** (где `_fd`, `_nickname`, и т.д.) и **добавить**:

```cpp
std::string _userModes;  // User modes (i, o, w, etc.)
```

Затем найти секцию с **public методами** и **добавить**:

```cpp
// User mode management
const std::string& getUserModes() const { return _userModes; }
void setUserMode(char mode, bool add);
bool hasUserMode(char mode) const;
```

---

## Изменение 4.2 - Реализовать методы user modes в Client

### В файле Client.cpp

Найти конструктор и **добавить инициализацию** `_userModes`:

```cpp
Client::Client(int fd)
    : _fd(fd), _authenticated(false), _registered(false),
      _shouldDisconnect(false), _userModes("")  // <-- добавить это
{
}
```

Затем **добавить в конец файла** эти методы:

```cpp
/**
 * @brief Set or unset a user mode
 * @param mode Mode character (i, o, w, etc.)
 * @param add True to add mode, false to remove
 */
void Client::setUserMode(char mode, bool add)
{
    if (add)
    {
        // Add mode if not already present
        if (_userModes.find(mode) == std::string::npos)
            _userModes += mode;
    }
    else
    {
        // Remove mode if present
        size_t pos = _userModes.find(mode);
        if (pos != std::string::npos)
            _userModes.erase(pos, 1);
    }
}

/**
 * @brief Check if user has a specific mode
 * @param mode Mode character to check
 * @return True if user has the mode
 */
bool Client::hasUserMode(char mode) const
{
    return _userModes.find(mode) != std::string::npos;
}
```

---



