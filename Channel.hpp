#ifndef CHANNEL_HPP
#define CHANNEL_HPP

#include <string>
#include <vector>
#include <map>
#include "User.hpp"
#include <iostream>
#include <sys/socket.h>

class Channel {
	private:
		std::string name;
		std::string topic;
		std::string password;
		size_t userLimit;
		bool inviteOnly;
		bool topicRestricted;
		std::vector<User *> clients;
		std::vector<User *> operators;
		std::vector<std::string> invitedUsers;
		Channel();
	public:
		const std::vector<User*>& getClients() const;
		void sendNotification(std::map<int, User>& allConnections, std::string kickMsg);
		void removeClient(std::string& name);
		bool hasClient(std::string& name) const;
		bool hasClient(User *user) const;
		bool isEmpty() const;
		~Channel();
		std::string getPassword() const;
		void sendToUsers(std::string& tmp, std::map<int, User>& allConnections, User &user);
		Channel(User *operatorM, const std::string &name);
		Channel(User *operatorM, const std::string &name, const std::string &pass);
		Channel(Channel const &other);
		Channel& operator=(Channel const &other);
		std::string getName() const;
		bool getInviteOnly() const;
		bool getTopicRestricted() const;
		bool hasPassword() const;
		bool checkPassword(const std::string &key) const;
		bool isFull() const;
		void setPassword(const std::string &k);
		void removePassword();
		void setUserLimit(size_t l);
		void removeUserLimit();
		void addClient(User *user);
		void removeClient(User *user);
		bool isOperator(User *user) const;
		void addOperator(User *user);
		void removeOperator(User *user);
	    std::string getTopic() const;
    	void setTopic(const std::string& newTopic);
		static std::string trimTopic(const std::string& str);
		void broadcastMessage(const std::string& msg);
		User* getUserByNick(const std::string& nick) const;
		bool isInvited(const std::string &nick) const;
		bool isInviteOnly();
		void setInviteOnly(bool value);
		void addInvitedUser(const std::string &nick);
		void removeInvitedUser(const std::string &nick);
		void setTopicRestricted(bool value);
};

#endif
