//
// Created by hcorrea- on 07-05-2024.
//

#pragma once

#include "main.hpp"
#include "client.hpp"

class user;

class channel {
	
	public:
		channel();
		~channel();
		bool		getTopicMode(void) const {return(topicMode);}
		bool		getInviteMode(void) const {return(inviteMode);}
		std::string	getPassword(void) const {return(passwd);}
		int			getMaxUsers(void) const {return(maxUsers);}
		void		setTopicMode(bool status){topicMode = status;}
		void		setInviteMode(bool status){inviteMode = status;}
		void		setPassword(const std::string &pass){passwd = pass;}
		void		setmaxUsers(int n){maxUsers = n;}
        std::string	getTopic(void) const {return(topic);}
        std::string	getTopicUser(void) const {return(topicSetBy);}
        std::string	getTopicNick(void) const {return(topicNick);}
        void		setTopic(const std::string &top){topic = top;}
        void        setTopicUser(const std::string &us){topicSetBy = us;}
        void        setTopicNick(const std::string &ni){topicNick = ni;}
		std::map<int, user> users;

	private:
		bool		topicMode;
		bool		inviteMode;
		int			maxUsers;
		std::string	passwd;
        std::string topic;
        std::string topicSetBy;
        std::string topicNick;
};