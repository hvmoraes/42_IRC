#pragma once

#include "main.hpp"
#include "parser.hpp"

class server;
class user;

// Command dispatch — routes a parsed IRC message to the appropriate handler.
void dispatchCommand(server *srv, int fd, const IRCMessage &msg);

// Individual command handlers (business logic separated from I/O).
void handleUser(server *srv, int fd, const IRCMessage &msg);
void handleNick(server *srv, int fd, const IRCMessage &msg);
void handlePass(server *srv, int fd, const IRCMessage &msg);
void handleJoin(server *srv, int fd, const IRCMessage &msg);
void handlePrivmsg(server *srv, int fd, const IRCMessage &msg);
void handleKick(server *srv, int fd, const IRCMessage &msg);
void handleInvite(server *srv, int fd, const IRCMessage &msg);
void handleTopic(server *srv, int fd, const IRCMessage &msg);
void handleMode(server *srv, int fd, const IRCMessage &msg);
void handleCap(server *srv, int fd, const IRCMessage &msg);
void handleQuit(server *srv, int fd, const IRCMessage &msg);
