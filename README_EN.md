# ft_irc

A lightweight IRC server written in C++ using a single-threaded, non-blocking I/O model.

This document is an evaluation-oriented project README: it explains the core IRC idea, server workflow, how to build/run, which client to use, and practical test scenarios with expected results.

---

## 1) Project goal

`ft_irc` implements an IRC server that can:
- accept multiple TCP clients concurrently,
- parse IRC commands,
- manage users/channels,
- send numeric replies and errors,
- support operator channel controls (`KICK`, `INVITE`, `TOPIC`, `MODE`).

No IRC client implementation is included (server only).

---

## 2) Core IRC principle (short)

IRC is a line-based text protocol over TCP.

- Each message ends with `\r\n`.
- Generic message format:

`[:prefix] <COMMAND> [params...] [:trailing with spaces]\r\n`

Example:
- Client → Server: `NICK alice\r\n`
- Client → Server: `USER alice 0 * :Alice Doe\r\n`
- Server → Client: `:ircserv 001 alice :Welcome ...\r\n`

Why buffering matters:
- TCP is a byte stream, so one `recv()` can contain partial command(s) or multiple commands at once.
- The server accumulates data in input buffers and processes only complete lines.

---

## 3) Subject-oriented design choices

- Single-threaded event loop with `poll()`.
- Non-blocking sockets (`O_NONBLOCK`) for listener and clients.
- Multiple clients handled in one loop.
- Deferred disconnect strategy (`markForDisconnect` + cleanup pass) to avoid container mutation hazards during iteration.
- Server-only scope (no server-to-server links).

---

## 4) High-level architecture

- `main.cpp`
  - validates args,
  - creates `Server`,
  - installs signal handlers,
  - starts `server.run()`.

- `src/network/Server.cpp`
  - socket setup (`socket/bind/listen`),
  - accept new clients,
  - poll loop,
  - read/write dispatch,
  - graceful shutdown.

- `src/network/Client.cpp`
  - per-client state: registration flags, input/output buffers, disconnect flags.

- `src/network/Channel.cpp`
  - channel membership, operators, topic/modes, invitations, limits.

- `src/protocol/Parser.cpp`
  - parses raw IRC line into command + parameters.

- `src/protocol/CommandHandler.cpp`
  - dispatches and executes IRC commands.

- `src/protocol/MessageBuilder.cpp`
  - builds compliant outgoing IRC replies/errors.

---

## 5) Supported commands

### Registration / connection
- `PASS`
- `NICK`
- `USER`
- `PING`
- `QUIT`

### Messaging
- `PRIVMSG`
- `NOTICE`

### Channels
- `JOIN`
- `PART`
- `TOPIC`
- `MODE`
- `INVITE`
- `KICK`
- `WHO`
- `CAP` (basic negotiation handling)

### Channel modes required by subject
- `+i` invite-only
- `+t` topic restricted to operators
- `+k` channel key/password
- `+o` give/remove channel operator
- `+l` user limit

---

## 6) Reference clients

### Recommended for evaluation: `irssi`
This is the reference IRC client for realistic protocol validation.

### Useful for low-level debugging: `nc` / `netcat`
Great to test raw lines, partial input, and edge cases quickly.

---

## 7) Build and run

From project root:

```bash
make re
./ircserv 6667 pass123
```

Expected startup:
- build succeeds,
- server prints startup message,
- process waits for incoming connections.

---

## 8) Quick manual tests with `nc` (and expected results)

Open terminal A:

```bash
./ircserv 6667 pass123
```

Open terminal B:

```bash
nc 127.0.0.1 6667
```

### Test 1 — Registration
Send:

```text
PASS pass123
NICK alice
USER alice 0 * :Alice Doe
```

Expected:
- `001`, `002`, `003`, `004` welcome sequence.

### Test 2 — Wrong password
Send:

```text
PASS wrong
```

Expected:
- `464` (password mismatch).

### Test 3 — Duplicate nickname
Connect a second client and reuse `alice`.

Expected:
- `433` (nickname in use).

### Test 4 — Partial command handling
Send fragmented input (example script):

```bash
{ printf 'PA'; sleep 1; printf 'SS pass123\r\n'; } | nc 127.0.0.1 6667
```

Expected:
- command is processed only after full line (`\r\n`) is received,
- server stays stable.

### Test 5 — Multiple commands in one burst

```bash
printf 'PING :one\r\nPING :two\r\n' | nc 127.0.0.1 6667
```

Expected:
- two independent PONG responses.

### Test 6 — Channel flow
Client 1:

```text
JOIN #room
```

Client 2:

```text
JOIN #room
PRIVMSG #room :hello
```

Expected:
- both join successfully,
- message is broadcast to other channel members.

### Test 7 — Mode checks
In channel operator session:

```text
MODE #room +i
```

Third client tries:

```text
JOIN #room
```

Expected:
- `473` invite-only error.

Then operator:

```text
INVITE nick3 #room
```

Expected:
- invited client can join.

### Test 8 — Graceful disconnect
- close a client with `Ctrl+D` or `QUIT :bye`.

Expected:
- server remains running,
- disconnected client is cleaned without crashing the loop.

### Command / expected code or result

| Command | Expected code/result |
|---|---|
| `PASS mypass` + `NICK` + `USER` | `001`, `002`, `003`, `004` (welcome) |
| `PASS` after registration | `462` (`ERR_ALREADYREGISTERED`) |
| `NICK 1abc` / `NICK bad*nick` | `432` (`ERR_ERRONEOUSNICKNAME`) |
| `NICK <already used>` | `433` (`ERR_NICKNAMEINUSE`) |
| `JOIN #chan` (success) | `JOIN` + `353` + `366` (+ `332` or `331`) |
| `JOIN` without parameters | `461 JOIN` |
| `JOIN` on `+i` channel without invite | `473` (`ERR_INVITEONLYCHAN`) |
| `JOIN` on `+k` channel with missing/wrong key | `475` (`ERR_BADCHANNELKEY`) |
| `JOIN` on full `+l` channel | `471` (`ERR_CHANNELISFULL`) |
| `PART #chan` when not in channel | `442` (`ERR_NOTONCHANNEL`) |
| `PRIVMSG #chan :text` (success) | Message is delivered to channel members |
| `PRIVMSG #chan` without text | `412` (`ERR_NOTEXTTOSEND`) |
| `PRIVMSG` without recipient | `411` (`ERR_NORECIPIENT`) |
| `PRIVMSG` to channel without membership | `404` (`ERR_CANNOTSENDTOCHAN`) |
| `NOTICE ...` | Delivered or silently ignored (typically no errors) |
| `MODE #chan` | `324` (current channel modes) |
| `MODE #chan +i/+k/+l/+t/+o` by operator | Mode is applied and broadcast |
| `MODE #chan ...` by non-operator | `482` (`ERR_CHANOPRIVSNEEDED`) |
| `TOPIC #chan` | `332` (topic exists) or `331` (no topic) |
| `TOPIC #chan :new` on `+t` channel by non-operator | `482` |
| `KICK #chan user :r` by operator | `KICK` message is broadcast in channel |
| `KICK ...` by non-operator | `482` |
| `INVITE nick #chan` success | `341` to inviter + `INVITE` to target |
| `INVITE` for user already in channel | `443` (`ERR_USERONCHANNEL`) |
| `INVITE` for non-existent nick | `401` (`ERR_NOSUCHNICK`) |
| `PING token` | `PONG ... :token` |
| `WHO #chan` / `WHO nick` | `352` (one or multiple) + `315` (`RPL_ENDOFWHO`) |
| Unknown command | `421` (`ERR_UNKNOWNCOMMAND`) |
| `QUIT :reason` | `QUIT` broadcast in channels + disconnect |
| `Ctrl+D` in `nc` (EOF) | Clean disconnect, server keeps running |

---

## 9) Real-client test with `irssi`

```bash
irssi -c 127.0.0.1 -p 6667 -w pass123 -n alice
```

Expected:
- successful registration,
- can join channels and exchange messages,
- operator commands behave according to modes/permissions.

---

## 10) Main expected evaluation outcomes

- No blocking behavior per connected client.
- No crash on malformed/partial input.
- Correct IRC numeric errors for invalid flows.
- Correct channel permission behavior for operator-only actions.
- Proper cleanup on quit/hangup/error paths.

---

## 11) Notes

- This project targets IRC server behavior for educational evaluation.
- Optional extras (bots/files/etc.) are outside mandatory scope.