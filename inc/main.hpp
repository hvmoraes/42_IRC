//
// Created by ldiogo on 03-05-2024.
//

#pragma once

#include <vector>
#include <poll.h>
#include <sys/epoll.h>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cerrno>
#include <fcntl.h>
#include <map>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "client.hpp"
#include "channel.hpp"
#include "server.hpp"
#include "parser.hpp"
#include "commands.hpp"

class user;
class server;

ssize_t send_user(int socket, const void *buffer, size_t length, int flags, server *srv = NULL);
ssize_t	send_all(server *server, const void *buffer, size_t length, int flags, const std::string &channel);
std::string trimWhitespace(const std::string &str);
