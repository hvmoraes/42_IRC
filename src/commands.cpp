#include "../inc/commands.hpp"

// ---------- Dispatch table ----------

void dispatchCommand(server *srv, int fd, const IRCMessage &msg)
{
	// During login phase, only allow registration commands
	int status = srv->users[fd].getStatus();
	if (status < 4)
	{
		// HexChat sends USER, NICK, PASS in a batch (possibly with CAP LS)
		if (msg.command == "CAP")
		{
			handleCap(srv, fd, msg);
			return;
		}
		if (msg.command == "USER")
		{
			handleUser(srv, fd, msg);
			return;
		}
		if (msg.command == "NICK")
		{
			handleNick(srv, fd, msg);
			return;
		}
		if (msg.command == "PASS")
		{
			handlePass(srv, fd, msg);
			return;
		}
		// Netcat login (starts with /PASS)
		if (msg.command == "/PASS")
		{
			handlePass(srv, fd, msg);
			return;
		}
		return;
	}

	// Fully authenticated — dispatch to command handlers
	if (msg.command == "JOIN")
		handleJoin(srv, fd, msg);
	else if (msg.command == "PRIVMSG")
		handlePrivmsg(srv, fd, msg);
	else if (msg.command == "KICK")
		handleKick(srv, fd, msg);
	else if (msg.command == "INVITE")
		handleInvite(srv, fd, msg);
	else if (msg.command == "TOPIC")
		handleTopic(srv, fd, msg);
	else if (msg.command == "MODE")
		handleMode(srv, fd, msg);
	else if (msg.command == "NICK")
		handleNick(srv, fd, msg);
	else if (msg.command == "QUIT")
		handleQuit(srv, fd, msg);
}

// ---------- Login commands ----------

void handleUser(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.empty())
	{
		send_user(fd, "Please enter a valid username: USER <username>\r\n", 48, 0, srv);
		return;
	}
	if (!srv->users[fd].getUsername().empty())
	{
		send_user(fd, "You may not reregister\r\n", 23, 0, srv);
		return;
	}
	std::string username = msg.params[0];
	std::string trimmed = trimWhitespace(username);
	if (trimmed.empty())
	{
		send_user(fd, "Please enter a valid username: USER <username>\r\n", 48, 0, srv);
		return;
	}
	// Extract just the first word (username part)
	std::size_t sp = trimmed.find(' ');
	if (sp != std::string::npos)
		trimmed = trimmed.substr(0, sp);
	srv->users[fd].setUsername(trimmed);
	std::cout << "Username set to: |" << trimmed << "|\n";

	// Advance login state based on current flow
	int status = srv->users[fd].getStatus();
	if (status == 0)
	{
		// HexChat flow: USER is first, advance to 1
		srv->users[fd].setStatus(1);
		srv->users[fd].setFromNc(0);
	}
	else if (status == 1)
	{
		// Netcat flow: after PASS, USER is second
		srv->users[fd].setStatus(2);
	}
	std::cout << "status = " << srv->users[fd].getStatus() << "\n";
}

void handleNick(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.empty())
	{
		send_user(fd, "Please enter a valid nickname: NICK <nickname>\r\n", 48, 0, srv);
		return;
	}
	std::string nick = trimWhitespace(msg.params[0]);
	if (nick.empty())
	{
		send_user(fd, "Please enter a valid nickname: NICK <nickname>\r\n", 48, 0, srv);
		return;
	}
	std::size_t sp = nick.find(' ');
	if (sp != std::string::npos)
		nick = nick.substr(0, sp);

	std::string oldNick = srv->users[fd].getNickname();
	if (oldNick == nick)
		return;

	if (!oldNick.empty())
	{
		srv->users[fd].setNickname(nick);
		std::string message = ":" + oldNick + "!" + srv->users[fd].getUsername() + " NICK " + nick + "\r\n";
		send_user(fd, message.c_str(), message.size(), 0, srv);
	}
	else
		srv->users[fd].setNickname(nick);

	std::cout << "Nickname set to: |" << nick << "|\n";

	// Advance login state
	int status = srv->users[fd].getStatus();
	if (status == 1)
	{
		// HexChat flow: NICK is second
		srv->users[fd].setStatus(2);
	}
	else if (status == 2 && srv->users[fd].getFromNc() == 1)
	{
		// Netcat flow: NICK is third
		srv->users[fd].setStatus(3);
	}
	std::cout << "status = " << srv->users[fd].getStatus() << "\n";

	// Check if login is complete
	if (srv->users[fd].getStatus() == 3)
	{
		send_user(fd, "Congratulations, you are now connected to the server!\r\n", 55, 0, srv);
		srv->users[fd].setStatus(4);
	}
}

void handlePass(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.empty())
	{
		if (srv->users[fd].getFromNc() == 1 || msg.command == "/PASS")
			send_user(fd, "Please enter a valid password: /PASS <password>\r\n", 49, 0, srv);
		else
			send_user(fd, "Please enter a valid password: PASS <password>\r\n", 48, 0, srv);
		return;
	}
	std::string pass = trimWhitespace(msg.params[0]);
	if (pass.empty())
	{
		send_user(fd, "Please enter a valid password: PASS <password>\r\n", 48, 0, srv);
		return;
	}
	std::size_t endPos = pass.find_first_of("\r\n");
	if (endPos != std::string::npos)
		pass = pass.substr(0, endPos);

	if (pass != srv->getPass())
	{
		std::cout << srv->getPass() << std::endl;
		std::cout << pass << std::endl;
		send_user(fd, "You've entered the wrong password, please try again\r\n", 53, 0, srv);
		return;
	}

	int status = srv->users[fd].getStatus();
	if (status == 2)
	{
		// HexChat flow: PASS is third
		srv->users[fd].setStatus(3);
		send_user(fd, "Congratulations, you are now connected to the server!\r\n", 55, 0, srv);
		srv->users[fd].setStatus(4);
	}
	else if (status == 0 && msg.command == "/PASS")
	{
		// Netcat flow: /PASS is first
		srv->users[fd].setFromNc(1);
		srv->users[fd].setStatus(1);
	}
	else
	{
		// Fallback
		srv->users[fd].setStatus(3);
		send_user(fd, "Congratulations, you are now connected to the server!\r\n", 55, 0, srv);
		srv->users[fd].setStatus(4);
	}
}

void handleCap(server *srv, int fd, const IRCMessage &msg)
{
	(void)srv;
	(void)fd;
	(void)msg;
	// CAP LS — silently ignore during registration
}

void handleQuit(server *srv, int fd, const IRCMessage &msg)
{
	(void)msg;
	std::string nick = srv->users[fd].getNickname();
	std::string username = srv->users[fd].getUsername();
	std::string quitMsg = ":" + nick + "!" + username + " QUIT :Quit\r\n";

	// Notify all channels the user was in
	std::map<std::string, channel*>::iterator it;
	for (it = srv->channels.begin(); it != srv->channels.end(); ++it)
	{
		if (it->second->users.find(fd) != it->second->users.end())
			send_all(srv, quitMsg.c_str(), quitMsg.size(), 0, it->first);
	}
}

// ---------- Channel commands ----------

void handleJoin(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.empty())
		return;
	std::string nick = srv->users[fd].getNickname();
	std::string username = srv->users[fd].getUsername();
	std::string channelName = trimWhitespace(msg.params[0]);
	std::size_t endPos = channelName.find_first_of("\t\n\r ");
	if (endPos != std::string::npos)
		channelName = channelName.substr(0, endPos);

	if (srv->channels.find(channelName) == srv->channels.end())
	{
		srv->channels[channelName] = new channel();
		srv->channels[channelName]->users[fd] = srv->users[fd];
		srv->channels[channelName]->users[fd].setOpStatus(true);
	}
	else if (srv->channels[channelName]->getInviteMode() == true)
	{
		std::string message = ": 473 " + nick + " " + channelName + " :Cannot join channel (+i)\r\n";
		send_user(fd, message.c_str(), message.size(), 0, srv);
		return;
	}
	else
		srv->channels[channelName]->users[fd] = srv->users[fd];

	std::string message = ":" + nick + "!" + username + " JOIN " + channelName + "\r\n";
	send_all(srv, message.c_str(), message.size(), 0, channelName);

	if (!srv->channels[channelName]->getTopic().empty())
	{
		std::string tmsg = ":" + channelName + " 332 " + nick + " " + channelName + " :" + srv->channels[channelName]->getTopic() + "\r\n";
		send_user(fd, tmsg.c_str(), tmsg.size(), 0, srv);
		tmsg = ":" + channelName + " 333 " + nick + " " + channelName + " " + srv->channels[channelName]->getTopicNick() + "!" + srv->channels[channelName]->getTopicUser() + " 1715866598\r\n";
		send_user(fd, tmsg.c_str(), tmsg.size(), 0, srv);
	}
}

void handlePrivmsg(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.size() < 2)
		return;
	std::string nick = srv->users[fd].getNickname();
	std::string username = srv->users[fd].getUsername();
	std::string target = msg.params[0];
	std::string text = msg.params[1];

	if (target.at(0) == '#')
	{
		// Channel message
		if (srv->channels.find(target) == srv->channels.end())
		{
			std::string err = ":Channel does not exist\r\n";
			send_user(fd, err.c_str(), err.size(), 0, srv);
			return;
		}
		for (std::size_t i = 0; i < srv->channels[target]->users.size(); i++)
		{
			if (srv->channels[target]->users[i].getSocket() != fd)
			{
				std::string out = ":" + nick + "!" + username + " PRIVMSG " + target + " :" + text + "\r\n";
				send_user(srv->channels[target]->users[i].getSocket(), out.c_str(), out.size(), 0, srv);
			}
		}
	}
	else
	{
		// Private message to a user
		std::map<int, user>::iterator it;
		for (it = srv->users.begin(); it != srv->users.end(); ++it)
		{
			if (it->second.getNickname() == target)
			{
				std::string out = ":" + nick + "!" + username + " PRIVMSG " + target + " :" + text + "\r\n";
				send_user(it->second.getSocket(), out.c_str(), out.size(), 0, srv);
				break;
			}
		}
	}
}

void handleKick(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.size() < 2)
		return;
	std::string channelName = msg.params[0];
	std::string targetName = msg.params[1];
	std::string nick = srv->users[fd].getNickname();
	std::string username = srv->users[fd].getUsername();

	std::size_t endPos = targetName.find_first_of("\t\n\r ");
	if (endPos != std::string::npos)
		targetName = targetName.substr(0, endPos);

	if (srv->channels.find(channelName) == srv->channels.end())
		return;
	if (srv->channels[channelName]->users[fd].getOpStatus() == false)
	{
		std::string err = ":" + channelName + " 482 " + nick + " " + channelName + " :You're not channel operator\r\n";
		send_user(fd, err.c_str(), err.size(), 0, srv);
		return;
	}

	for (std::size_t i = 0; i < srv->channels[channelName]->users.size(); i++)
	{
		std::string u = srv->channels[channelName]->users[i].getUsername();
		std::string n = srv->channels[channelName]->users[i].getNickname();
		if (u == targetName || n == targetName)
		{
			std::string message = ":" + nick + "!" + username + " KICK " + channelName + " " + targetName + " :" + nick + "\r\n";
			for (std::size_t j = 0; j < srv->channels[channelName]->users.size(); j++)
				send_user(srv->channels[channelName]->users[j].getSocket(), message.c_str(), message.size(), 0, srv);
			srv->channels[channelName]->users.erase(i);
			break;
		}
	}
}

void handleInvite(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.size() < 2)
		return;
	std::string nick = srv->users[fd].getNickname();
	std::string nameOp = msg.params[0];
	std::string channelName = msg.params[1];

	bool userExists = false;
	for (std::size_t i = 0; i < srv->users.size(); i++)
	{
		if (srv->users[i].getNickname() == nameOp)
		{
			userExists = true;
			break;
		}
	}
	if (!userExists)
	{
		std::string err = ":" + channelName + " 401 " + nick + " " + nameOp + " :No such nick\r\n";
		send_user(fd, err.c_str(), err.size(), 0, srv);
		return;
	}
	if (srv->channels.find(channelName) == srv->channels.end())
	{
		std::string err = ":" + channelName + " 403 " + nick + " " + channelName + " :No such channel\r\n";
		send_user(fd, err.c_str(), err.size(), 0, srv);
		return;
	}
	if (srv->channels[channelName]->getInviteMode() && !srv->channels[channelName]->users[fd].getOpStatus())
	{
		std::string err = ":" + channelName + " 482 " + nick + " " + nameOp + " " + channelName + " :You're not channel operator\r\n";
		send_user(fd, err.c_str(), err.size(), 0, srv);
		return;
	}

	std::size_t endPos = nameOp.find_first_of("\t\n\r ");
	if (endPos != std::string::npos)
		nameOp = nameOp.substr(0, endPos);

	for (std::size_t i = 0; i < srv->channels[channelName]->users.size(); i++)
	{
		if (srv->channels[channelName]->users[i].getNickname() == nameOp)
		{
			std::string err = ":" + channelName + " 443 " + nick + " " + nameOp + " " + channelName + " :is already on channel\r\n";
			send_user(fd, err.c_str(), err.size(), 0, srv);
			return;
		}
	}

	for (std::size_t i = 0; i < srv->users.size(); i++)
	{
		if (srv->users[i].getNickname() == nameOp)
		{
			std::string invMsg = ":" + nick + "!" + srv->users[fd].getUsername() + " INVITE " + nameOp + " " + channelName + "\r\n";
			send_user(srv->users[i].getSocket(), invMsg.c_str(), invMsg.size(), 0, srv);
			std::string ack = ":" + channelName + " 341 " + nick + " " + nameOp + " " + channelName + "\r\n";
			send_user(fd, ack.c_str(), ack.size(), 0, srv);
			return;
		}
	}
}

void handleTopic(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.empty())
		return;
	std::string nick = srv->users[fd].getNickname();
	std::string username = srv->users[fd].getUsername();
	std::string channelName = msg.params[0];

	if (srv->channels.find(channelName) == srv->channels.end())
		return;

	std::string topic;
	if (msg.params.size() >= 2)
		topic = msg.params[1];

	std::size_t endPos = topic.find_first_of("\t\n\r");
	if (endPos != std::string::npos)
		topic = topic.substr(0, endPos);

	if (topic.empty())
	{
		if (srv->channels[channelName]->getTopic().empty())
		{
			std::string out = ":" + channelName + " 331 " + nick + " " + channelName + " :No topic is set.\r\n";
			send_user(fd, out.c_str(), out.size(), 0, srv);
			return;
		}
		std::string out = ":" + channelName + " 332 " + nick + " " + channelName + " :" + srv->channels[channelName]->getTopic() + "\r\n";
		send_user(fd, out.c_str(), out.size(), 0, srv);
		out = ":" + channelName + " 333 " + nick + " " + channelName + " " + srv->channels[channelName]->getTopicNick() + "!" + srv->channels[channelName]->getTopicUser() + " 1715866598\r\n";
		send_user(fd, out.c_str(), out.size(), 0, srv);
		return;
	}

	if (srv->channels[channelName]->getTopicMode() && !srv->channels[channelName]->users[fd].getOpStatus())
	{
		std::string err = ":" + channelName + " 482 " + nick + " " + channelName + " :You're not channel operator\r\n";
		send_user(fd, err.c_str(), err.size(), 0, srv);
		return;
	}

	std::string out = ":" + nick + "!" + username + " TOPIC " + channelName + " :" + topic + "\r\n";
	send_all(srv, out.c_str(), out.size(), 0, channelName);
	srv->channels[channelName]->setTopic(topic);
	srv->channels[channelName]->setTopicNick(nick);
	srv->channels[channelName]->setTopicUser(username);
}

void handleMode(server *srv, int fd, const IRCMessage &msg)
{
	if (msg.params.size() < 2)
		return;
	std::string channelName = msg.params[0];
	std::string flag = msg.params[1];
	std::string nameOp;
	if (msg.params.size() >= 3)
		nameOp = msg.params[2];

	if (srv->channels.find(channelName) == srv->channels.end())
		return;

	// Delegate to existing user::mode() for full mode handling
	// Reconstruct buffer for backward compat with mode() implementation
	std::string buf = msg.command + " " + channelName + " " + flag;
	if (!nameOp.empty())
		buf += " " + nameOp;
	buf += "\r\n";

	// Use a stack-allocated copy for the C-string
	char tmp[MAX_MSG_LEN];
	std::strncpy(tmp, buf.c_str(), MAX_MSG_LEN - 1);
	tmp[MAX_MSG_LEN - 1] = '\0';

	srv->users[fd].mode(srv, tmp, fd);
}
