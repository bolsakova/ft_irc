# ft_irc — project explanation (up to 15 minutes)

## 1) What this project is
`ft_irc` is a single-threaded IRC server in C++ using a non-blocking I/O model.

Core idea:
- one thread;
- one event loop (`poll`);
- many clients handled without `thread`;
- IRC commands processed via `CommandHandler`.

---

## 2) Module architecture
- `main.cpp` — entry point, args, signals, starts `server.run()`.
- `src/network/Server.cpp` — network layer, `poll` loop, accept/read/write/disconnect.
- `src/network/Client.cpp` — client state (buffers, flags, registration).
- `src/network/Channel.cpp` — channels, members, operators, modes.
- `src/protocol/CommandHandler.cpp` — IRC command handling.
- `src/protocol/Parser.cpp` — raw command parsing.
- `src/protocol/MessageBuilder.cpp` — reply/numeric message formatting.

---

## 3) Startup flow (Start block in the diagram)
1. Validate arguments (`argc == 3`).
2. Create `Server(port, password)`.
3. Inside constructor:
   - `ignore_sigpipe()`;
   - `initSocket(port)`:
     - `socket`, `setsockopt(SO_REUSEADDR)`, `bind`, `listen`, `O_NONBLOCK`;
   - create `CommandHandler`;
   - add `listen_fd` to `m_poll_fds` with `POLLIN`.
4. Register `SIGINT/SIGTERM`.
5. Enter `run()`.

Important: the signal handler only sets `g_stop_requested = 1`.

---

## 4) Main loop (Main loop block in the diagram)
The server core is `while (m_running)`:
- if `m_poll_fds` is empty, this is an error case;
- call `poll(m_poll_fds.data(), m_poll_fds.size(), -1)`;
- on `EINTR`:
  - if `g_stop_requested == 1` → `stop()`;
  - otherwise continue loop;
- iterate over all `fd` with events (`revents != 0`).

---

## 5) listen_fd branch: new connections
If `fd == m_listen_fd` and `POLLIN` is set:
- `acceptClient()` accepts all pending connections in a loop;
- each client socket gets `O_NONBLOCK`;
- add `pollfd` to `m_poll_fds`;
- create a `Client` object in `m_clients`.

---

## 6) client_fd branch: read/write/errors
For client sockets:

### POLLIN → `receiveData(fd)`
- `recv` in a loop until `EAGAIN/EWOULDBLOCK`;
- data accumulates in `inbuf`;
- complete IRC commands are extracted;
- each command goes to `CommandHandler`;
- if response exists, enable `POLLOUT`.

### POLLOUT → `sendData(fd)`
- send from `outbuf`;
- partial sends are handled correctly;
- if `outbuf` becomes empty, disable `POLLOUT`.

### POLLERR/POLLNVAL
- client is marked for disconnect (`markForDisconnect`).

### POLLHUP
- set `markPeerClosed`;
- if `outbuf` is empty, mark for disconnect immediately;
- if not empty, flush remaining data first via `POLLOUT`.

---

## 7) Why disconnect is deferred
The server does not remove clients immediately inside `m_poll_fds` iteration, to avoid unsafe container modification during traversal.

Flow:
1. set flags in handlers (`markForDisconnect`, `markPeerClosed`);
2. after processing all `fd`, call `cleanupDisconnectedClients()`;
3. only there call `disconnectClient(fd)`.

This matches the final diagram node: **All fds processed? → cleanupDisconnectedClients()**.

---

## 8) IRC logic (short)
`CommandHandler` processes core commands:
- registration: `PASS`, `NICK`, `USER`;
- channel operations: `JOIN`, `PART`, `TOPIC`, `MODE`, `INVITE`, `KICK`;
- messaging: `PRIVMSG`, `NOTICE`;
- service: `PING`, `WHO`, `CAP`, `QUIT`.

Replies are built in IRC format, including numeric replies/errors.

---

## 9) Graceful shutdown
On `SIGINT/SIGTERM`:
- handler sets `g_stop_requested`;
- `poll` is interrupted (`EINTR`), then `run` calls `stop()`;
- shutdown notice is sent;
- clients are disconnected;
- `listen_fd` is closed and containers are cleaned.

---

## 10) Key defense points (short)
- Why one thread: simpler model, no races, lower overhead.
- Why `poll`: efficient event-driven architecture.
- Why non-blocking: one client cannot stall the server.
- Why `unique_ptr`: RAII and safe memory management.
- Why deferred disconnect: safe container updates.

---

## 11) 30-second summary
This project implements a classic IRC server event loop:
- `poll` + non-blocking sockets;
- clear network/protocol separation;
- correct client lifecycle (connect → process → deferred disconnect);
- safe shutdown via signal flag + `EINTR`.

The architecture is stable, readable, and suitable for a training IRC server.