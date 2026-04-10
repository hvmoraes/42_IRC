//
// Created by ldiogo on 03-05-2024.
//

#pragma once

#include "main.hpp"
#include "client.hpp"
#include "channel.hpp"

#define BUFFER_SIZE 4096
#define MAX_CLIENTS 128
#define MAX_NICK_LEN 9
#define MAX_CHANNEL_LEN 50
#define MAX_MSG_LEN 512
#define RATE_LIMIT_WINDOW 10
#define RATE_LIMIT_MAX_MSGS 20

class channel;
class user;

class server
{
    public:
        server(int port, char *password, const char *certFile = NULL, const char *keyFile = NULL);
        ~server();
        void removeUserFromChannels(int fd);

        // epoll management
        int addToEpoll(int fd);
        int removeFromEpoll(int fd);

        int socket_id;
        int epoll_fd;
        std::map<std::string, channel*> channels;
        std::map<int, user> users;
        std::map<int, std::string> recv_buffers; // per-client message accumulation
        std::map<int, SSL*> ssl_sessions;        // per-client SSL session
        SSL_CTX *ssl_ctx;
        bool tls_enabled;
        std::string getPass() const { return password; }
    private:
        void initSSL(const char *certFile, const char *keyFile);
        sockaddr_in IP;
        int clientSocket;
        std::string password;
};