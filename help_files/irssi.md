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
