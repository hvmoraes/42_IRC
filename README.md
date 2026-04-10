# ft_irc

An IRC server built from scratch in C++98, implementing the core of the Internet Relay Chat protocol over raw TCP sockets with optional TLS encryption.

## Architecture

```
Client (irssi / nc)
        │
        ▼
┌───────────────────┐
│   epoll() loop    │  ← Non-blocking I/O multiplexing
│   (main.cpp)      │
├───────────────────┤
│   server          │  ← Socket lifecycle, user/channel registry
│   (server.cpp)    │
├────────┬──────────┤
│ client │ channel  │  ← Per-connection state / per-room state
└────────┴──────────┘
```

## Tech Stack

| Component | Detail |
|-----------|--------|
| Language | C++98 |
| I/O Model | `epoll()` — single-threaded event loop |
| Transport | TCP (SOCK_STREAM), optional TLS (OpenSSL) |
| Build | Makefile, `cc` with `-Wall -Wextra -Werror` |

## Supported IRC Commands

`PASS` · `NICK` · `USER` · `JOIN` · `PART` · `PRIVMSG` · `KICK` · `INVITE` · `TOPIC` · `MODE` (`+i`, `+t`, `+k`, `+o`, `+l`) · `QUIT`

## Building & Running

```bash
make
./ft_irc <port> <password>
```

Connect with any IRC client:
```bash
irssi -c 127.0.0.1 -p <port> -w <password>
```

## Key Engineering Decisions

- **Non-blocking sockets with `O_NONBLOCK` + `epoll()`** ([src/server.cpp](src/server.cpp), [src/main.cpp](src/main.cpp)): The server never blocks on any single client. All file descriptors — including the listening socket — are set non-blocking, and `epoll()` multiplexes reads across all connections in a single thread.

- **Optional TLS support** ([src/server.cpp](src/server.cpp) `initSSL()`): When certificate and key files are provided, the server wraps connections in TLS via OpenSSL (TLSv1.2+), keeping plaintext mode available for development.

- **RFC-compliant message parser** ([src/parser.cpp](src/parser.cpp)): A dedicated parser handles IRC message framing — prefix extraction, command identification, and parameter splitting with trailing-parameter support (`:` prefix for multi-word messages).

- **Safe disconnect handling** ([src/main.cpp](src/main.cpp) `disconnect_client()`): Client disconnection saves the fd before erasing from the user map, then iterates channels with safe iterator advancement to avoid use-after-erase and iterator invalidation bugs.

- **Channel memory lifecycle** ([src/server.cpp](src/server.cpp) `removeUserFromChannels()`): Channels are heap-allocated. When the last user leaves, the channel is deleted immediately. The server destructor also sweeps remaining channels to prevent leaks.

- **Rate limiting** ([src/client.cpp](src/client.cpp) `isRateLimited()`): A per-client sliding window rate limiter (20 messages per 10 seconds) prevents flooding without complex token-bucket overhead.
