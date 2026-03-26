# ft_irc Defense Script (Simple English, 2 Speakers)

## Team split
- **Lima**: `network/` (sockets, poll loop, recv/send, disconnect flow)
- **Tanja**: `protocol/` (parsing, command handlers, numeric replies, IRC rules)

## 1) Opening (30–40 sec)

### Lima
"We built an IRC server in C++. We split the project into two clear parts so the code stays clean and easy to debug."

### Tanja
"Lima handles the network layer: sockets, poll, reading and sending data. I handle the protocol layer: command parsing, validation, and replies."

### Lima
"Because of this split, if a bug is about connection/event flow, it is network. If a bug is about command behavior or reply codes, it is protocol."

---

## 2) Lima part (Network flow)

"I will explain how the server works on the network side.

In `main`, we validate arguments: port and password.
Then we initialize the listening socket with `socket`, `setsockopt`, `bind`, and `listen`.
We set sockets to non-blocking mode, so one slow client never blocks the whole server.

After that, we run one main loop with `poll`.
`poll` waits for events on all file descriptors at once.

If event is on `listen_fd`, we `accept` a new client, set it non-blocking, and add it to poll.

If event is `POLLIN` on a client, we read with `recv` in a loop until `EAGAIN`.
Incoming bytes are appended to per-client input buffer.
When a full IRC command is available, we pass it to protocol layer.

If event is `POLLOUT`, we send bytes from output buffer.
If send is partial, we keep the rest in buffer for next writable event.
When output buffer becomes empty, we disable `POLLOUT` for that fd.

On `POLLERR`/`POLLNVAL`, we disconnect immediately.
On `POLLHUP`, we mark peer closed. If output buffer is empty, disconnect now; otherwise we flush pending data first, then disconnect.

For stop signals (`SIGINT`/`SIGTERM`), we exit loop safely, disconnect clients, and close listening socket."

### Transition to Tanja
"Now Tanja will explain what happens to each IRC command after network layer extracts a full line."

---

## 3) Tanja part (Protocol flow)

"I handle IRC logic.

When network gives us a full command line, parser splits it into command, params, and trailing part.
Then `CommandHandler` routes to the correct handler: `PASS`, `NICK`, `USER`, `JOIN`, `PRIVMSG`, `MODE`, `KICK`, `INVITE`, `TOPIC`, `QUIT`, and others.

Core rule: we validate first, execute second.
We check parameters, registration state, permissions, channel existence, nickname uniqueness, etc.

If command is valid, we build the correct IRC reply and append it to client output buffer.
If invalid, we send numeric error reply (for example `433`, `482`, `461`).

`QUIT` flow is important:
1. Build reason (`Client exited` by default).
2. Broadcast QUIT to channels.
3. Queue `ERROR :Closing Link...` to quitting client.
4. Mark client for disconnect.
5. Actual socket close is done by network layer after output buffer is flushed.

So protocol decides **what** to send, network decides **when/how** to send it safely."

---

## 4) Demo plan (3–5 min)

1. Start server: `./ircserv 6667 pass42`
2. Connect two clients (`nc` or `irssi`), register with `PASS/NICK/USER`
3. Both join `#ops`
4. Send `PRIVMSG` in channel
5. Operator commands: `MODE +i`, `INVITE`, `KICK`
6. `QUIT :bye` on one client
7. Show correct errors for invalid actions (example: non-operator gets `482`)

---

## 5) Common evaluator questions (short answers)

### Why only one poll loop?
"Subject requirement, and it is the standard event-driven design for many clients without blocking."

### What if a command arrives in pieces?
"We buffer bytes per client and process only complete commands."

### Why not close socket immediately on QUIT?
"To avoid losing final replies. We flush output buffer first, then disconnect."

### How do you prevent duplicate nicknames?
"Before setting nick, we check all connected clients (excluding current fd on nick change)."

### Where is responsibility split?
"Network handles transport/events. Protocol handles IRC rules/replies/permissions."

### Why may irssi not show `ERROR :Closing Link`?
"Some clients hide that raw line in UI. With `nc`, raw server output is visible directly."

---

## 6) Final closing line

"We separated transport logic and IRC logic on purpose. This keeps the server stable under multiple clients and keeps command behavior predictable and easy to test."
