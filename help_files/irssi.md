## irssi (Key Shortcuts)

---

## Window/Channel navigation

| Клавиша 			     | Action
|------------------------|-------------------
| from `Alt+1` to `Alt+9`| Switch to windows 1-9
| `Alt+0`			     | Switch to window 10
| `Alt+Q` до `Alt+O`     | Switch to windows 11-19
| `Ctrl+N`			     | Next window
| `Ctrl+P`			     | Previous window
| `Alt+A`			     | Switch to active window (new messages)
| `/window 3`		     | Switch to window 3

---

## Text editing

| Key			    | Action
|-------------------|-------------------
| `Ctrl+A`			| Beginning of the string
| `Ctrl+E`			| End of the string
| `Ctrl+U`			| Remove from cursor to beginning
| `Ctrl+K`			| Remove from cursor to the end
| `Ctrl+W`			| Remove word back
| `Ctrl+Y`			| Insert last removed text
| `Alt+Backspace`	| Remove word back
| `Alt+D`			| Remove word forward
| `Tab`				| Auto-addition nick/command


---

## History scrolling

| Key				| Action
|-------------------|-----------------------
| `Page Up`			| Scrolling up
| `Page Down`		| Scrolling down
| `Ctrl+Home`		| Buffer beginning
| `Ctrl+End`		| Buffer end (to new messages)
| `↑` (arrow up)	| Previous command from history
| `↓` (arrow down)	| Next command from history

---

## Manage windows

| Key				    | Action
|-----------------------|-----------------------
| `Ctrl+L`				| Update screen (redraw)
| `/clear`				| Clear current window
| `/window close`		| Close current window
| `/window new`			| Create new window
| `/window name <name>` | Name the window

---

## Special functions

| Key			    | Action
|-------------------|-------------------
| `Alt+X`			| Switch input string (hide/show)
| `Ctrl+B`			| Insert format code (bold)
| `Ctrl+C`			| Insert color code
| `Ctrl+_`			| Insert underline code
| `Ctrl+]`			| Insert italic code
| `Ctrl+Z`			| Suspend irssi (return: `fg`)

---

## Useful commands for testing

| Command								| Action
|---------------------------------------|---------------------------------------
| `/connect localhost 6667 pass nick`	| Connect to server
| `/disconnect`							| Disconnect from current server
| `/reconnect`							| Reconnect
| `/join #channel`						| Join to the channel
| `/part #channel`						| Leave the channel
| `/names`								| show users in channel
| `/who #channel`						| WHO request 
| `/whois nick`							| Info about user
| `/msg nick text`						| Send private message
| `/notice nick text`					| Send NOTICE
| `/query nick`							| Open private window with user
| `/topic #channel New topic`			| Set topic
| `/kick #channel nick reason`			| Kick user
| `/invite nick #channel`				| Invite user to the channel
| `/mode #channel +i`					| Set channel mode
| `/mode nick +i`						| Set user mode
| `/away reason`						| Set away reason
| `/away`								| Remove away reason
| `/quit message`						| Quit from irssi
| `/exit`								| The same as /quit

---

## Settings management

| Command						| Action
|-------------------------------|-------------------------------
| `/set`						| Show all settings
| `/set nick alice`				| Set nick
| `/set real_name Alice Cooper`	| Set real name
| `/save`						| Save configuration
| `/reload`						| Reload configuration
| `/help command`				| Command helper

---

## Hot keys for WSL/Linux

| Key		        | Action
|-------------------|-------------------
| `Shift+Insert`	| Insert from buffer
| `Ctrl+Shift+C`	| Copy (in some terminals)
| `Ctrl+Shift+V`	| Insert (in some terminals)

---

## Useful tipps

### Quick switch between channels:
```
Alt+1  - Window with server status
Alt+2  - Usually 1st channel (#test)
Alt+3  - 2nd channel
```

### Autocomplete:
```
/j #te<Tab>     → /join #test
bob: he<Tab>    → bob: hello (complete nick in the beginning of the message)
```

### Open private chat:
```
/query bob
# Now just write in that window, PRIVMSG bob appears automatically
```

### Режим paste (для многострочного текста):
```
/set paste_join_multiline OFF
# Позволяет вставлять несколько строк одновременно
```

---

## Тестовый workflow

**Типичная последовательность для тестирования:**

```
1. Запустить: irssi
2. Подключиться: /connect localhost 6667 mypass alice
3. Войти в канал: /join #test
4. Отправить сообщение: Hello everyone!
5. Переключить окно: Alt+1 (статус), Alt+2 (#test)
6. Открыть второй терминал и подключить bob
7. Тестировать команды между alice и bob
8. Выйти: /quit
```

---



Тестирование от 24.03.2026 в nc

1) MODE #ops +k 
=> нет ответа, тихий баг (должна выводиться 461 Not enough parameters ошибка по RFC)

2) MODE #ops +l 
=> нет ответа, тихий баг (должна выводиться 461 Not enough parameters ошибка по RFC)

3) MODE #ops +o
=> нет ответа, тихий баг (должна выводиться 461 Not enough parameters ошибка по RFC)

4) Для NC команда QUIT:

 - тест без интерактива
 printf 'PASS pass42\r\nNICK t1\r\nUSER t1 0 * :T1\r\nQUIT :bye\r\n' | nc -C -w 2 127.0.0.1 6667

 - тест с интерактивом
 В интерактивном nc после QUIT закрывай stdin через Ctrl+D, а не Enter/пробел.


---

## NC testing

**Условные обозначения**
1. `S` — терминал сервера  
2. `C1` — `nc` клиент `alice`  
3. `C2` — `nc` клиент `bob`  
4. `C3` — `nc` клиент `carol`  
5. `C4` — `nc` клиент `dave`  
6. Порт: `6667`, пароль: `pass42`

**Важно**
1. Лучше запускать `nc -C 127.0.0.1 6667` (чтобы отправлялся CRLF).  
2. Если `-C` нет, тогда можно через `printf '...\r\n' | nc ...`.  
3. В ответах возможны отличия в порядке строк (из-за poll), это нормально.  
4. Ключевое: numeric-коды и смысл ответа.

---

## 1) Старт и регистрация

### S (сервер)
```bash
cd /mnt/c/Users/tanju/OneDrive/42studies/irc
make
./ircserv 6667 pass42
```

Ожидаемо в `S`: сервер поднялся, слушает порт.

### C1/C2/C3/C4
```bash
nc -C 127.0.0.1 6667
```

### C1 (alice)
```text
PASS pass42
NICK alice
USER alice 0 * :Alice A
```

Ожидаемо в `C1`:
```text
:ircserv 001 alice :Welcome to the Internet Relay Network alice!alice@localhost
:ircserv 002 alice :Your host is ircserv, running version 1.0
:ircserv 003 alice :This server was created 2025-12-21
:ircserv 004 alice :ircserv 1.0 io itkol
```

### C2 (bob)
```text
PASS pass42
NICK bob
USER bob 0 * :Bob B
```

Ожидаемо: те же `001..004` для `bob`.

### C3 (carol)
```text
PASS pass42
NICK carol
USER carol 0 * :Carol C
```

Ожидаемо: `001..004` для `carol`.

### C4 (dave)
```text
PASS pass42
NICK dave
USER dave 0 * :Dave D
```

Ожидаемо: `001..004` для `dave`.

---

## 2) База канала для operator-тестов

### C1
```text
JOIN #ops
```
Ожидаемо:
1. `:alice!alice@localhost JOIN :#ops`
2. `353` со списком, где `@alice`
3. `366` конец списка
4. `331` “No topic is set” (если темы нет)

### C2
```text
JOIN #ops
```
Ожидаемо:
1. JOIN-сообщение у участников
2. `353` (`@alice bob`)
3. `366`
4. `331` или `332` (если тема уже была)

### C3
```text
JOIN #ops
```
Ожидаемо аналогично.

---

## 3) Обязательные команды операторов

## 3.1 KICK

### Негатив: не-оператор не может
### C2
```text
KICK #ops carol :nope
```
Ожидаемо в `C2`:
```text
:ircserv 482 bob #ops :You're not channel operator
```

### Позитив: оператор может
### C1
```text
KICK #ops carol :cleanup
```
Ожидаемо в `C1/C2/C3`:
```text
:alice!alice@localhost KICK #ops carol :cleanup
```
`carol` после этого вне канала.

---

## 3.2 INVITE

### Поставим +i (invite-only)
### C1
```text
MODE #ops +i
```
Ожидаемо в участниках канала:
```text
:alice!alice@localhost MODE #ops +i
```

### Негатив: не-оператор не может INVITE в +i
### C2
```text
INVITE carol #ops
```
Ожидаемо в `C2`:
```text
:ircserv 482 bob #ops :You're not channel operator
```

### Позитив: оператор приглашает
### C1
```text
INVITE carol #ops
```
Ожидаемо:
1. В `C1`: `341`  
```text
:ircserv 341 alice :carol #ops
```
2. В `C3`: приглашение  
```text
:alice!alice@localhost INVITE carol :#ops
```

### C3
```text
JOIN #ops
```
Ожидаемо: успешный JOIN (без 473).

---

## 3.3 TOPIC

### Без +t: не-оператор может менять
### C2
```text
TOPIC #ops :topic by bob
```
Ожидаемо в канале:
```text
:bob!bob@localhost TOPIC #ops :topic by bob
```

### Включаем +t
### C1
```text
MODE #ops +t
```
Ожидаемо:
```text
:alice!alice@localhost MODE #ops +t
```

### Негатив: не-оператор меняет тему при +t
### C2
```text
TOPIC #ops :bob tries again
```
Ожидаемо:
```text
:ircserv 482 bob #ops :You're not channel operator
```

### Позитив: оператор меняет тему
### C1
```text
TOPIC #ops :operator topic
```
Ожидаемо broadcast:
```text
:alice!alice@localhost TOPIC #ops :operator topic
```

### Проверка чтения темы
### C2
```text
TOPIC #ops
```
Ожидаемо:
```text
:ircserv 332 bob #ops :operator topic
```

---

## 3.4 MODE i/t/k/l/o

### Негатив: не-оператор меняет mode
### C2
```text
MODE #ops +i
```
Ожидаемо:
```text
:ircserv 482 bob #ops :You're not channel operator
```

### +k / -k
### C1
```text
MODE #ops +k secret
MODE #ops
```
Ожидаемо:
1. Broadcast mode:
```text
:alice!alice@localhost MODE #ops +k secret
```
2. Просмотр mode (`324`) с `+...k...` и ключом.

### Проверка key на JOIN
### C2
```text
PART #ops :test key
JOIN #ops wrong
```
Ожидаемо:
```text
:ircserv 475 bob #ops :Cannot join channel (+k)
```
### C2
```text
JOIN #ops secret
```
Ожидаемо: успешный JOIN.

### C1
```text
MODE #ops -k
```
Ожидаемо:
```text
:alice!alice@localhost MODE #ops -k
```

### +l / -l
### C1
```text
MODE #ops +l 3
MODE #ops
```
Ожидаемо:
1. Broadcast `MODE #ops +l 3`
2. В `324` есть `l 3`

Если в канале уже 3 участника, попробуй C4:
### C4
```text
JOIN #ops
```
Ожидаемо:
```text
:ircserv 471 dave #ops :Cannot join channel (+l)
```

Снять лимит:
### C1
```text
MODE #ops -l
```
Ожидаемо: broadcast `MODE #ops -l`.

### +o / -o
### C1
```text
MODE #ops +o bob
```
Ожидаемо:
```text
:alice!alice@localhost MODE #ops +o bob
```

Проверка, что bob теперь оператор:
### C2
```text
MODE #ops -t
```
Ожидаемо: успех (broadcast от `bob`).

Снять права:
### C1
```text
MODE #ops -o bob
```
Ожидаемо broadcast `MODE #ops -o bob`.

Проверка, что bob снова не-оператор:
### C2
```text
MODE #ops +i
```
Ожидаемо `482`.

---

## 4) Критичные edge-cases (где вероятны баги)

Эти кейсы специально на “тихие” ошибки.

### C1
```text
MODE #ops +k
```
Нормально по RFC ожидать `461` (Not enough parameters).  
Если вообще нет ответа и mode не меняется — фиксируй как баг (silent failure).

### C1
```text
MODE #ops +l
MODE #ops +l abc
```
Нормально ожидать ошибку (`461`/валидация).  
Если тишина — баг.

### C1
```text
MODE #ops +o
```
Нормально ожидать ошибку недостатка параметров.  
Если тишина — баг.

---

## 5) Остальные команды проекта (после operator-части)

## 5.1 PING/PONG
### C1
```text
PING 12345
```
Ожидаемо:
```text
:ircserv PONG ircserv :12345
```

## 5.2 PRIVMSG user
### C1
```text
PRIVMSG bob :hello bob
```
Ожидаемо в `C2`:
```text
:alice!alice@localhost PRIVMSG bob :hello bob
```

## 5.3 PRIVMSG channel
### C1
```text
PRIVMSG #ops :hello channel
```
Ожидаемо у остальных участников канала (не у отправителя):
```text
:alice!alice@localhost PRIVMSG #ops :hello channel
```

## 5.4 NOTICE
### C1
```text
NOTICE bob :notice text
```
Ожидаемо у `C2`: NOTICE-сообщение.  
Если цель не существует, обычно ошибок не будет (и это ок для NOTICE).

## 5.5 WHO
### C1
```text
WHO #ops
```
Ожидаемо:
1. Одна или несколько строк `352`
2. Конец:
```text
:ircserv 315 alice :#ops :End of WHO list
```
(формат может немного отличаться, главное `352` + `315`)

## 5.6 PART
### C2
```text
PART #ops :bye
```
Ожидаемо в канале:
```text
:bob!bob@localhost PART #ops :bye
```

## 5.7 QUIT
### C3
```text
QUIT :leaving
```
Ожидаемо:
1. В каналах: `QUIT` от `carol`
2. В `C3` перед закрытием:
```text
ERROR :Closing Link: carol (Quit: leaving)
```

---

## 6) Мини-прогон для irssi (кросс-проверка)

В `irssi`:
```text
/connect 127.0.0.1 6667 pass42 alice2
/join #ops2
/mode #ops2 +t
/topic #ops2 :set by op
/who #ops2
/quit
```

Ожидаемо:
1. Успешное подключение после PASS/NICK/USER (irssi может отправлять CAP — сервер должен отвечать корректно).
2. JOIN + список имен.
3. MODE/TOPIC видны в окне канала.
4. WHO возвращает список участников и end-of-WHO.

---

## 7) Что фиксировать как проблему сразу

1. Нет numeric-ошибки там, где команда некорректна (особенно `MODE +k/+l/+o` без параметров).  
2. Не-оператор может делать operator-действия в канале с ограничениями.  
3. Неправильные коды: например, ожидался `482`, а пришло другое/ничего.  
4. Падение сервера, зависание, отвал сокета после обычной команды.

Если хочешь, следующим сообщением дам тебе готовую табличку-шаблон для копипаста (markdown), куда ты просто будешь вставлять фактический вывод по шагам.
