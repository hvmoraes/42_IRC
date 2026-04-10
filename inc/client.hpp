//
// Created by ldiogo on 03-05-2024.
//

#pragma once

#include "server.hpp"

class server;

class user
{
    public:
        user(int newSocket);
        user() : clientSocket(-1), status(0), from_nc(0), isOp(false), msgCount(0), lastMsgTime(0) {}
        ~user();
        int clientSocket;
        std::string getNickname() const { return nickname; }
        std::string getUsername() const { return username; }
        bool getOpStatus() const { return isOp; }
        int getSocket() const { return clientSocket; }
        void setNickname(const std::string &newNickname) { nickname = newNickname; }
        void setUsername(const std::string &newUsername) { username = newUsername; }
        void setStatus(int newStatus) { status = newStatus; }
        int getStatus() const { return status; }
		void setFromNc(int newFromnc) { from_nc = newFromnc; }
		int getFromNc() const { return from_nc; }
        void setOpStatus(bool newStatus) { isOp = newStatus; }
        bool isRateLimited();
		void	check_operator(char *buf, int fd, server *server);
		void	modeOperator(server *server, user &newOp, const std::string &channel, const std::string &flag);
		int		modeInvite(server *server, const std::string &channel, const std::string &flag);
		int		modeTopic(server *server, const std::string &channel, const std::string &flag);
		void	mode(server *server, char *buffer, int fd);
		int		modeCheck(server *server, const std::string &channel, int fd);
		int		modePassword(server *server, const std::string &channel, const std::string &flag, const std::string &key);
		int		modeLimit(server *server, const std::string &channel, const std::string &flag, const std::string &amount);
    protected:
        int status;
		int from_nc;
        std::string nickname;
        std::string username;
		bool	isOp;
        int msgCount;
        time_t lastMsgTime;
};