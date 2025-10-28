#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include <map>
#include <sys/poll.h>
#include <cstdio>
#include "User.hpp"
#include "Channel.hpp"
#include <set>
#include <sstream>

class Server {
	private:
		void sendJoinReply(User&, Channel*);
		std::string trim(const std::string& str);
		void removeUserFromAllChannels(User *user);
		std::set<Channel *> channels;
		int port;
		std::string pass;
		std::map<int, User> allConnections;
		Server();
		std::vector<std::string> split(const std::string& res, const std::string& delim);
		int processCommand(User &user, std::string command);
		void setIfRegistered(User &user, int clientFd);
		std::vector<pollfd> fdClients;
		void sendMsg(User &user, std::string command, int listeningSocket, int countOfChars);
		User* getUserByNickname(const std::string& nickname);
	public:
		void startServer();
		Server(int port, std::string pass);
		~Server();
		Server(Server const &other);
		Server& operator=(Server const &other);
};

#endif
