## Problems & Fixing

1. [x] **Порядок PASS/NICK/USER в nc**

	- программа должна работать при любом порядке этих команд
	- влияет на резервацию имени, когда клиент выходит, и корректное удаление `fd` соответственно

	=> добавила проверку, является клиент после `PASS` полностью зарегистрированным в `handlePass()`, теперь не важно в каком порядке выполнять команды

2. [x] **/pass command (irssi) not found**

	`/pass pass` -> `Irssi: Unknown command: pass`

	- в IRSSI пароль передается при подключении, а не отдельной командой
	- НЕТ команды `/pass` после подключения клиента, поэтому это является корректным поведением

	=> это поведение IRSSI клиента

3. [] **QUIT (nc) и /quit (irssi) (с причиной и без) совершают выход только после Enter (nc) или отправки любого сообщения (irssi)**

	- 

4. [x] **Нормально ли, что все время приходят оповещения в сервере PING/PONG для каждого клиента?**

	- 

5. [] **Incorrect message building for /kick (irssi)**

	`/kick #test bob Bye!` -> сообщение у bob `bob was kicked from #test by alice`

	- должно быть: `You were kicked...`

6. [] **/user command (irssi) not found**

	`/user pip (irssi)` -> `userhost Unknown command`

	- предположение: то же самое, что с `/pass`

7. [] **Incorrect message building for /invite (irssi)**
	
	`/invite alima #test (irssi)` -> `Inviting alima #test to`

	- должно быть: `Inviting alima to #test` (порядок слов)

8. [] **Клиенты после команды NOTICE (nc) и /notice (irssi) получают оповещения только после Enter (nc) или отправки любого сообщения (irssi)**

	- такая проблема уже была решена с `PRIVMSG` и прочими командами, которые отправляют оповещения другим клиентам

9. [x] **Incorrect message building for JOIN (nc)**

	1 `JOIN #test`

	2 `:alice!alice@localhost JOIN :#test`
	3 `:ircserv 353 alice = #test :@alice`
	4 `:ircserv 366 alice :#test :End of /NAMES list`
	5 `:ircserv 331 alice :#test :No topic is set`

	- 4-5: перед `#test` не должно быть двоеточия

	=> в `handlejoin()` заменила `sendNumeric()` на `sendReply()`

10. [] **Not useful output in Server terminal**

	1 `Broadcasting to 1 members`
	2 `Sending to fd 4 (alice)`

	- использовалось для убеждения, что оповещения вообще отправляются и нужным клиентам
	- нужно убрать перед эвалюейшном

11. [] **Incorrect expected output for /join (irssi)**

	1 `-!- bob [bob@localhost] has joined #test`
	2 `-!- Irssi: #test: Total of 2 nicks [1 ops, 0 halfops, 0 voices, 1 normal]`
	3 `-!- Topic for #test: No topic is set`
	4 `-!- Channel #test created on [дата]`
	5 `[@test] @alice bob`

	- 1: есть в канале у bob клиента (окно 2)
	- 2: отсутствует
	- 3: `-!- #test :No topic is set` (окно 1)
	- 4-5: отсутствуют

12. [] **NAMES command (nc) not found**

	`NAMES #test` -> `:ircserv 421 alice NAMES :Unknown command`

	*Проблема:*
	- не написана, но в irssi работает

	*Предложение чата:*
	- для минимальной работы в nc: можно обойтись без отдельной NAMES
	- для полноценного IRC сервера: NAMES команда обязательна по RFC 1459 (но не в сабджекте)

	*Почему работает в irssi?*
	- когда в irssi вводят /names: irssi НЕ отправляет команду NAMES на сервер! irssi просто показывает список пользователей, который он уже знает из предыдущих NAMES replies

	*Почему стоит или не стоит реализовать?*
	✅ 5 минут работы (код уже дал)
	✅ Полноценная поддержка IRC протокола
	✅ Удобство тестирования в nc
	✅ Совместимость с другими клиентами
	✅ Требование для оценки 42 (по subject)

	❌ Тестируете только через irssi
	❌ Не заботитесь о RFC 1459 соответствии
	❌ Не нужно поддерживать другие IRC клиенты

13. [x] **Пробелы в NAMES (irssi)**

	`< charlie>`
	`[ bob  ]`

	- irssi просто форматирует отображение для красоты (выравнивание в колонки)

	=> это не ошибка

