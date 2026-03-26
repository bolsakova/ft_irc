*This project has been created as part of the 42 curriculum by tbolsako and aokhapki*

# Description
=============

ft_irc is a multi-client IRC server written in C++ as part of the 42 curriculum.

The project goal is to build a real server that handles multiple simultaneous connections in one process, without threads and without fork, using non-blocking sockets with poll().

Our implementation focuses on core IRC behavior from RFC 1459:

  - client registration with PASS, NICK, USER
  - channel join and management
  - private and channel messaging
  - operator actions such as KICK, INVITE, TOPIC, and MODE
  - support for both netcat and irssi testing workflows

# Project Structure
===================

### Root
  - main.cpp: program entry point, argument validation, signal handling, server startup
  - Makefile: build and utility targets
  - README.md: project documentation
  - inc/: headers
  - src/: source files
  - help_files/: defense material

### inc
  - inc/network: Server, Client, Channel interfaces
  - inc/protocol: Parser, CommandHandler, MessageBuilder, Replies interfaces

### src
  - src/network: poll loop, socket lifecycle, channel/client runtime behavior
  - src/protocol: parsing IRC lines, command dispatch, reply generation

### help_files
  - testing guides for netcat and irssi
  - project flow diagrams

# Work Division and Responsibility Strategy
===========================================

## Work Division
----------------

### tbolsako:
  - protocol-side architecture and behavior
  - command parsing and validation logic
  - command semantics and numeric reply behavior
  - protocol consistency checks during integration
  - theoretical validation of reference client behavior (irssi)

### aokhapki:
  - server-side architecture and runtime flow
  - socket setup, non-blocking configuration, and poll-based event loop
  - client lifecycle and connection handling
  - channel/client state management and runtime stability
  - integration of server components into a coherent execution loop

## Responsibility Strategy
--------------------------

  - Project is splited by layers (protocol vs server core) to reduce merge conflicts and improve ownership clarity.
  - Interface contracts were agreed early to keep parallel development safe.
  - Final integration and validation were performed jointly.

# What to Pay Attention To
===========================================

  - IRC command framing must be line-based and correctly terminated
  - Registration order must be consistent: PASS, then NICK, then USER
  - Server must stay with multiple connected clients in non-blocking mode
  - Basic channel permissions and operator commands should behave predictably
  - `irssi` and `nc` are both useful, but `irssi` may show UI-side messages that are not server bugs

# Instructions
==============

1. Build:
  make

2. Run server:
  ./ircserv 6667 pass42

3. Connect with netcat in another terminal:
  nc 127.0.0.1 6667

4. Register client:
  PASS pass42
  NICK alice
  USER alice 0 * :Alice

  => Expected welcome sequence in nc (001, 002, 003, 004):
  :server 001 alice :Welcome to the IRC Network
  :server 002 alice :Your host is server
  :server 003 alice :This server was created today
  :server 004 alice server 1.0 o o

5. Basic channel flow:
  JOIN #test
  PRIVMSG #test :Hello, everyone!
  NOTICE #test :Server check

  => Expected successful join and visible message flow

6. Reference-client check with irssi:
  irssi
  /connect 127.0.0.1 6667 pass42

  => in irssi registration is done at one time with connecting to server, so no such messages as in netcat there

7. Stop server:
  Ctrl+C in server terminal

# Notes

If irssi shows *"No such channel" message* right after connection, this is often irssi trying to auto-join previously saved channels. This is usually a client-side workflow behavior, not a server crash.

# Resources

1. Internet Relay Chat Protocol (RFC 1459):
  https://www.rfc-editor.org/rfc/rfc1459.html#section-1.1

2. poll() documentation:
  https://pubs.opengroup.org/onlinepubs/009696799/functions/poll.html

3. Practical IRC server article:
  https://medium.com/@afatir.ahmedfatir/small-irc-server-ft-irc-42-network-7cee848de6f9

4. IRC overview:
  https://en.wikipedia.org/wiki/IRC

5. IRC client selection:
  https://libera.chat/guides/clients

6. IRC channels and command management:
  https://medium.com/@mohamedsarda/ft-irc-channels-and-command-management-ff1ff3758a0b

# AI Usage Disclosure

  - Claude.ai - used for defense presentation planning and narrative structure; to evaluate and improve task distribution strategy and responsibility boundaries.

  - ChatGPT - theory clarification was used for understanding and explaining the role of irssi as the chosen reference client during testing and defense presentation.

  * AI was was used for planning, structuring, and debugging orientation only. Project implementation, integration, and validation were carried out by the team.
