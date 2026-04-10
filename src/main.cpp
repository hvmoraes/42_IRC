#include "../inc/main.hpp"

std::string trimWhitespace(const std::string &str)
{
	std::size_t start = str.find_first_not_of(" \t\r\n");
	if (start == std::string::npos)
		return "";
	std::size_t end = str.find_last_not_of(" \t\r\n");
	return str.substr(start, end - start + 1);
}

ssize_t	send_user(int socket, const void *buffer, size_t length, int flags, server *srv)
{
    if (srv && srv->tls_enabled && srv->ssl_sessions.count(socket))
    {
        SSL *ssl = srv->ssl_sessions[socket];
        int totalSent = 0;
        const char *ptr = (const char *)buffer;
        while (totalSent < static_cast<int>(length))
        {
            int ret = SSL_write(ssl, ptr + totalSent, length - totalSent);
            if (ret <= 0)
                return -1;
            totalSent += ret;
        }
        return totalSent;
    }

    ssize_t totalSentBytes = 0;
    const char *bufferPtr = (const char*) buffer;

    while (totalSentBytes < static_cast<ssize_t>(length))
    {
        ssize_t sentBytes = send(socket, bufferPtr + totalSentBytes, length - totalSentBytes, flags);
        if (sentBytes == -1)
        {
            return -1;
        }

        totalSentBytes += sentBytes;
    }

    return totalSentBytes;
}

ssize_t	send_all(server *server, const void *buffer, size_t length, int flags, const std::string &channel)
{
	ssize_t totalSentBytes = 0;

	for (std::size_t j = 0; j < server->channels[channel]->users.size(); j++)
	{
		totalSentBytes += send_user(server->channels[channel]->users[j].getSocket(), buffer, length, flags, server);
	}
	return (totalSentBytes);
}

static void accept_new_client(server *srv)
{
    int newClientSocket = accept(srv->socket_id, NULL, NULL);
    if (newClientSocket == -1)
    {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            std::cerr << "Can't accept client!" << std::endl;
        return;
    }
    if (srv->users.size() >= MAX_CLIENTS)
    {
        std::string msg = "Server is full, try again later\r\n";
        send(newClientSocket, msg.c_str(), msg.size(), 0);
        close(newClientSocket);
        return;
    }
    int flags = fcntl(newClientSocket, F_GETFL, 0);
    fcntl(newClientSocket, F_SETFL, flags | O_NONBLOCK);

    if (srv->tls_enabled)
    {
        // Temporarily set blocking for SSL handshake
        fcntl(newClientSocket, F_SETFL, flags & ~O_NONBLOCK);
        SSL *ssl = SSL_new(srv->ssl_ctx);
        SSL_set_fd(ssl, newClientSocket);
        int ret = SSL_accept(ssl);
        if (ret <= 0)
        {
            std::cerr << "SSL handshake failed for fd " << newClientSocket << std::endl;
            ERR_print_errors_fp(stderr);
            SSL_free(ssl);
            close(newClientSocket);
            return;
        }
        fcntl(newClientSocket, F_SETFL, flags | O_NONBLOCK);
        srv->ssl_sessions[newClientSocket] = ssl;
    }

    user newUser(newClientSocket);
    srv->users[newClientSocket] = newUser;
    srv->recv_buffers[newClientSocket] = "";
    srv->addToEpoll(newClientSocket);
    std::string welcome = "Welcome to the server!\r\nPlease enter the server password: /PASS <password>\r\n";
    send_user(newClientSocket, welcome.c_str(), welcome.size(), 0, srv);
}

static void disconnect_client(int fd, server *srv)
{
    std::cout << "Client disconnected (fd " << fd << ")" << std::endl;
    srv->removeFromEpoll(fd);
    if (srv->ssl_sessions.count(fd))
    {
        SSL_shutdown(srv->ssl_sessions[fd]);
        SSL_free(srv->ssl_sessions[fd]);
        srv->ssl_sessions.erase(fd);
    }
    close(fd);
    srv->removeUserFromChannels(fd);
    srv->users.erase(fd);
    srv->recv_buffers.erase(fd);
}

int main(int argc, char **argv)
{
    if(argc != 3 && argc != 5)
    {
        std::cerr << "Usage: ./ft_irc <port> <password> [<cert.pem> <key.pem>]" << std::endl;
        return (1);
    }
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Error: port must be between 1 and 65535" << std::endl;
        return (1);
    }

    const char *certFile = (argc == 5) ? argv[3] : NULL;
    const char *keyFile  = (argc == 5) ? argv[4] : NULL;

    server serverT(port, argv[2], certFile, keyFile);
    server *srv = &serverT;

    struct epoll_event events[MAX_CLIENTS + 1];
    char buffer[BUFFER_SIZE];

    while (true)
    {
        int nfds = epoll_wait(srv->epoll_fd, events, MAX_CLIENTS + 1, 100);

        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;
            std::cerr << "Error in epoll_wait(). Quitting" << std::endl;
            break;
        }

        for (int n = 0; n < nfds; ++n)
        {
            int fd = events[n].data.fd;

            if (fd == srv->socket_id)
            {
                accept_new_client(srv);
                continue;
            }

            if (!(events[n].events & EPOLLIN))
                continue;

            std::memset(buffer, 0, BUFFER_SIZE);
            int bytesRead;
            if (srv->tls_enabled && srv->ssl_sessions.count(fd))
                bytesRead = SSL_read(srv->ssl_sessions[fd], buffer, BUFFER_SIZE - 1);
            else
                bytesRead = recv(fd, buffer, BUFFER_SIZE - 1, 0);

            if (bytesRead <= 0)
            {
                disconnect_client(fd, srv);
                continue;
            }
            buffer[bytesRead] = '\0';

            if (srv->users[fd].isRateLimited())
            {
                std::string msg = "ERROR :Flooding detected, slow down\r\n";
                send_user(fd, msg.c_str(), msg.size(), 0);
                continue;
            }

            // Accumulate data in per-client buffer and extract complete messages
            srv->recv_buffers[fd] += std::string(buffer, bytesRead);
            std::vector<std::string> messages = extractMessages(srv->recv_buffers[fd]);

            for (std::size_t m = 0; m < messages.size(); ++m)
            {
                IRCMessage msg = parseIRCMessage(messages[m]);
                if (msg.command.empty())
                    continue;
                dispatchCommand(srv, fd, msg);
            }
        }
    }
}
