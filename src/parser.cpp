#include "../inc/parser.hpp"

// Extract complete \r\n-delimited messages from the accumulated buffer.
// Leaves any incomplete trailing data in `buffer`.
std::vector<std::string> extractMessages(std::string &buffer)
{
	std::vector<std::string> messages;
	std::string::size_type pos;
	while ((pos = buffer.find("\r\n")) != std::string::npos)
	{
		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 2);
		if (!line.empty())
			messages.push_back(line);
	}
	// Also handle bare \n (netcat sends \n without \r)
	while ((pos = buffer.find("\n")) != std::string::npos)
	{
		std::string line = buffer.substr(0, pos);
		buffer.erase(0, pos + 1);
		// Strip trailing \r if present
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (!line.empty())
			messages.push_back(line);
	}
	return messages;
}

// Parse a single IRC line into prefix, command, and params.
// Format: [':' prefix ' '] command {' ' param} [' :' trailing]
IRCMessage parseIRCMessage(const std::string &line)
{
	IRCMessage msg;
	std::string::size_type idx = 0;

	// Optional prefix
	if (!line.empty() && line[0] == ':')
	{
		std::string::size_type sp = line.find(' ', 1);
		if (sp != std::string::npos)
		{
			msg.prefix = line.substr(1, sp - 1);
			idx = sp + 1;
		}
	}

	// Skip leading spaces
	while (idx < line.size() && line[idx] == ' ')
		idx++;

	// Command
	{
		std::string::size_type sp = line.find(' ', idx);
		if (sp == std::string::npos)
		{
			msg.command = line.substr(idx);
			return msg;
		}
		msg.command = line.substr(idx, sp - idx);
		idx = sp + 1;
	}

	// Params
	while (idx < line.size())
	{
		while (idx < line.size() && line[idx] == ' ')
			idx++;
		if (idx >= line.size())
			break;
		if (line[idx] == ':')
		{
			// Trailing parameter — rest of the line
			msg.params.push_back(line.substr(idx + 1));
			break;
		}
		std::string::size_type sp = line.find(' ', idx);
		if (sp == std::string::npos)
		{
			msg.params.push_back(line.substr(idx));
			break;
		}
		msg.params.push_back(line.substr(idx, sp - idx));
		idx = sp + 1;
	}
	return msg;
}
