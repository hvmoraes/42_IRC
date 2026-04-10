#pragma once

#include <string>
#include <vector>

struct IRCMessage {
	std::string prefix;
	std::string command;
	std::vector<std::string> params;
};

IRCMessage parseIRCMessage(const std::string &line);
std::vector<std::string> extractMessages(std::string &buffer);
