//
// Created by ldiogo on 03-05-2024.
//

#include "../inc/server.hpp"

server::server(int port, char *password, const char *certFile, const char *keyFile)
{
    this->password = std::string(password);
    this->ssl_ctx = NULL;
    this->tls_enabled = false;

    if (certFile && keyFile)
        initSSL(certFile, keyFile);

    this->socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (this->socket_id == -1) {
        std::cerr << "Can't create a socket!" << std::endl;
        exit(EXIT_FAILURE);
    }
    int optval = 1;
    if (setsockopt(socket_id, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
    {
        std::cerr << "Error setting socket options" << std::endl;
        close(socket_id);
        exit(EXIT_FAILURE);
    }

    int flags = fcntl(socket_id, F_GETFL, 0);
    fcntl(socket_id, F_SETFL, flags | O_NONBLOCK);

    this->IP.sin_family = AF_INET;
    this->IP.sin_addr.s_addr = INADDR_ANY;
    this->IP.sin_port = htons(port);
    if (bind(this->socket_id, (sockaddr *)&this->IP, sizeof(this->IP)) == -1) {
        std::cerr << "Can't bind to port " << port << std::endl;
        close(socket_id);
        exit(EXIT_FAILURE);
    }
    if (listen(this->socket_id, SOMAXCONN) == -1) {
        std::cerr << "Can't listen!" << std::endl;
        close(socket_id);
        exit(EXIT_FAILURE);
    }

    // Initialize epoll
    this->epoll_fd = epoll_create1(0);
    if (this->epoll_fd == -1) {
        std::cerr << "Can't create epoll instance!" << std::endl;
        close(socket_id);
        exit(EXIT_FAILURE);
    }
    addToEpoll(this->socket_id);

    std::cout << "IRC server listening on port " << port << std::endl;
}

int server::addToEpoll(int fd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = fd;
    return epoll_ctl(this->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

int server::removeFromEpoll(int fd)
{
    return epoll_ctl(this->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
}

void server::removeUserFromChannels(int fd)
{
    std::map<std::string, channel *>::iterator it = channels.begin();
    while (it != channels.end())
    {
        it->second->users.erase(fd);
        if (it->second->users.empty())
        {
            delete it->second;
            channels.erase(it++);
        }
        else
            ++it;
    }
}

server::~server()
{
    for (std::map<int, SSL*>::iterator it = ssl_sessions.begin(); it != ssl_sessions.end(); ++it)
        SSL_free(it->second);
    ssl_sessions.clear();
    for (std::map<int, user>::iterator it = users.begin(); it != users.end(); ++it)
        close(it->first);
    for (std::map<std::string, channel *>::iterator it = channels.begin(); it != channels.end(); ++it)
        delete it->second;
    channels.clear();
    recv_buffers.clear();
    close(this->epoll_fd);
    close(this->socket_id);
    if (this->ssl_ctx)
        SSL_CTX_free(this->ssl_ctx);
}

void server::initSSL(const char *certFile, const char *keyFile)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    this->ssl_ctx = SSL_CTX_new(SSLv23_server_method());
    if (!this->ssl_ctx)
    {
        std::cerr << "Error creating SSL context" << std::endl;
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    // Disable old protocols, allow TLSv1.2+
    SSL_CTX_set_options(this->ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);

    if (SSL_CTX_use_certificate_file(this->ssl_ctx, certFile, SSL_FILETYPE_PEM) <= 0)
    {
        std::cerr << "Error loading certificate: " << certFile << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(this->ssl_ctx);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(this->ssl_ctx, keyFile, SSL_FILETYPE_PEM) <= 0)
    {
        std::cerr << "Error loading private key: " << keyFile << std::endl;
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(this->ssl_ctx);
        exit(EXIT_FAILURE);
    }
    if (!SSL_CTX_check_private_key(this->ssl_ctx))
    {
        std::cerr << "Private key does not match certificate" << std::endl;
        SSL_CTX_free(this->ssl_ctx);
        exit(EXIT_FAILURE);
    }
    this->tls_enabled = true;
    std::cout << "TLS enabled with certificate: " << certFile << std::endl;
}
