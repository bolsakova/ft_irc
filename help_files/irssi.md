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



Тестирование от 04.01.2026 в nc
1. TODO - удалить сообщения на сервере 
На настоящем сервере эти debug-сообщения не должны быть.

Это промежуточные отладочные (debug) выводы, которые помогали вам во время разработки. Реальный IRC сервер должен работать тихо — без вывода в консоль. Вместо этого логи обычно пишутся в файлы или в syslog.

1. + PRIVMSG между пользователями (личные сообщения как в канале так и просто отправляются/принимаются отлично):
Bob → nc: PRIVMSG a :Hi Alice!
Alice должна получить: :bob!bob@localhost PRIVMSG a :Hi Alice!
---
Bob → nc: PRIVMSG #test :Hello everyone!
Все получают: :bob!bob@localhost PRIVMSG #test :Hi everyone!


2. + TOPIC (установка/просмотр топика): 
Alice (оператор): TOPIC #test :Welcome to our channel!
Bob (не оператор): TOPIC #test :I try to change  (должна быть ошибка если +t / без +t может менять)

3. + MODE (режимы канала):
Alice: MODE #test +t        (только операторы могут менять топик)
Alice: MODE #test +i        (invite-only)
Alice: MODE #test +k secret (установить пароль)
Alice: MODE #test +l 5      (лимит пользователей)
Alice: MODE #test +o bob    (дать bob'у права оператора)

4. + KICK (выкинуть пользователя):
Alice: KICK #test bob :Bad behavior
Bob должен получить сообщение о kick и выйти из канала

5. + INVITE (пригласить в +i канал):
Alice: MODE #test +i
Alice: INVITE bob #test
Bob: JOIN #test  (должен зайти, т.к. приглашен)

6. + PART (выход из канала):
Bob: PART #test :Goodbye!

7. + QUIT (отключиться от сервера):
Bob: QUIT :See you later
QUIT работает отлично и с сообщением и без при выходе и без enter всем сразу приходит сообщение и происходит выход

8. проблема с правами в некоторых командах
