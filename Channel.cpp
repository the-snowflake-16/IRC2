#include "Channel.hpp"

bool Channel::hasClient(User *user) const {
	for (size_t i = 0; i < clients.size(); i++) {
		if (clients[i]->getFd() == user->getFd())
			return true;
	}
	return false;
}

bool Channel::hasClient(std::string& name) const {
	for (size_t i = 0; i < clients.size(); i++) {
		if (clients[i]->getNickname() == name)
			return true;
	}
	return false;
}

bool Channel::isEmpty() const {
	return clients.empty();
}

std::string Channel::getPassword() const {
	return this->password;
}

void Channel::sendToUsers(std::string& tmp, std::map<int, User>& allConnections, User &sender) {
	for (std::map<int, User>::iterator it = allConnections.begin(); it != allConnections.end(); ++it) {
		for (size_t i = 0; i < clients.size(); ++i) {
			if (clients[i]->getFd() == it->second.getFd()) {
				if (sender.getFd() != it->second.getFd()) {
					if (send(it->first, tmp.c_str(), tmp.size(), 0) == -1)
						std::cerr << "Error sending to " << it->second.getNickname() << std::endl;
				}
				break;
			}
		}
	}
}

std::string Channel::getName() const {
	return this->name;
}

Channel::Channel() {
}

Channel::~Channel() {
	clients.clear();
	operators.clear();
}

Channel::Channel(User *operatorM, const std::string &name) {
	this->name = name;
	this->userLimit = 0; //без лимита по участникам 
	this->topicRestricted = true; //только оператор (создатель канала) может менять тему
	this->inviteOnly = false; //любой пользователь может присоединиться, если не стоит ключ (+k) или лимит (+l)
	this->clients.push_back(operatorM);
	this->operators.push_back(operatorM);
}

Channel::Channel(User *operatorM, const std::string &name, const std::string &pass) {
	this->name = name;
	this->userLimit = 0; //без лимита по участникам
	this->topicRestricted = true; //только оператор (создатель канала) может менять тему
	this->inviteOnly = false; //любой пользователь может присоединиться, если не стоит ключ (+k) или лимит (+l)
	this->clients.push_back(operatorM);
	this->operators.push_back(operatorM);
	this->password = pass;
}

Channel::Channel(Channel const &other) {
	this->topic = other.topic;
	this->name = other.name;
	this->userLimit = other.userLimit;
	this->topicRestricted = other.topicRestricted;
	this->inviteOnly = other.inviteOnly;
	this->clients = other.clients;
	this->operators = other.operators;
	this->password = other.password;
}

Channel& Channel::operator=(Channel const &other) {
	if (&other != this) {
		this->topic = other.topic;
		this->name = other.name;
		this->userLimit = other.userLimit;
		this->topicRestricted = other.topicRestricted;
		this->inviteOnly = other.inviteOnly;
		this->clients = other.clients;
		this->operators = other.operators;
		this->password = other.password;
	}
	return *this;
}

bool Channel::getInviteOnly() const {
	return this->inviteOnly;
}

bool Channel::getTopicRestricted() const {
	return this->topicRestricted;
}

bool Channel::hasPassword() const {
	if (!this->password.empty())
		return true;
	return false;
}

bool Channel::checkPassword(const std::string &key) const {
	if (key == this->password)
		return true;
	return false;
}

bool Channel::isFull() const {
	if (this->userLimit == 0)
		return false;
	if (this->userLimit <= this->clients.size())
		return true;
	return false;
}

// void Channel::setInviteOnly(bool b) {
// 	 this->inviteOnly = b;
// }

void Channel::setTopicRestricted(bool b) {
	this->topicRestricted = b;
}

void Channel::setPassword(const std::string &k) {
	this->password = k;
}

void Channel::removePassword() {
	this->password = "";
}

void Channel::setUserLimit(size_t l) {
	if (l == 0) ;
	else if (l < this->clients.size() || l < 1) {
		std::cerr << "Your limit is too low" << std::endl;
		return ;
	}
	this->userLimit = l;
}

void Channel::removeUserLimit() {
	this->userLimit = 0;
}

void Channel::addClient(User *user) {
	for (std::vector<User *>::iterator it = this->clients.begin(); it != clients.end(); it++) {
		if ((*it)->getFd() == user->getFd())
			return ;
	}
	this->clients.push_back(user);
}

void Channel::removeClient(User *user) {
	for (std::vector<User *>::iterator it = this->clients.begin(); it != clients.end(); it++) {
		if ((*it)->getFd() == user->getFd()) {
			clients.erase(it);
			break;
		}
	}
}

const std::vector<User*>& Channel::getClients() const {
	return clients; 
}

void Channel::removeClient(std::string& name) {
	for (std::vector<User *>::iterator it = this->clients.begin(); it != clients.end(); it++) {
		if ((*it)->getNickname() == name) {
			clients.erase(it);
			break;
		}
	}
}

bool Channel::isOperator(User *user) const {
	for (std::vector<User *>::const_iterator it = this->operators.begin(); it != operators.end(); it++) {
		if ((*it)->getFd() == user->getFd())
			return true;
	}
	return false;
}

void Channel::addOperator(User *user) {
	for (std::vector<User *>::iterator it = this->operators.begin(); it != operators.end(); it++) {
		if ((*it)->getFd() == user->getFd())
			return ;
	}
	this->operators.push_back(user);
}

void Channel::removeOperator(User *user) {
	for (std::vector<User *>::iterator it = this->operators.begin(); it != operators.end(); it++) {
		if ((*it)->getFd() == user->getFd()) {
			it = operators.erase(it);
			break;
		}
	}
}

void Channel::sendNotification(std::map<int, User>& allConnections, std::string kickMsg) {
	for (std::map<int, User>::iterator it = allConnections.begin(); it != allConnections.end(); ++it) {
		for (size_t i = 0; i < clients.size(); ++i) {
			if (clients[i]->getFd() == it->second.getFd()) {
				send(it->first, kickMsg.c_str(), kickMsg.size(), 0);
                                break;
                        }
                }
        }
}

std::string Channel::getTopic() const { 
	return topic; 
}

void Channel::setTopic(const std::string& newTopic) { 
	topic = newTopic; 
}

std::string Channel::trimTopic(const std::string& str) {
	size_t start = 0;
	size_t end = str.size();

	while (start < end && (isspace(str[start]) || str[start] == ':')) {
		++start;
	}
	
	while (end > start && (isspace(str[end - 1]) || str[end - 1] == ':')) { 
		--end;
	}
    return str.substr(start, end - start);
}

void Channel::broadcastMessage(const std::string& msg) {
    for (size_t i = 0; i < clients.size(); ++i) {
        int fd = clients[i]->getFd();
        send(fd, msg.c_str(), msg.size(), 0);
    }
}

User* Channel::getUserByNick(const std::string& nick) const {
    // clients — это, допустим, std::vector<User*>
    for (std::vector<User*>::const_iterator it = clients.begin(); it != clients.end(); ++it) {
        if ((*it)->getNickname() == nick) {
            return *it; // нашли пользователя
        }
    }
    return NULL; // nullptr ещё нет в C++98, используем NULL
}

bool Channel::isInvited(const std::string &nick) const {
	for (size_t i = 0; i < invitedUsers.size(); ++i) {
		if (invitedUsers[i] == nick)
			return true;
	}
	return false;
}

void Channel::addInvitedUser(const std::string &nick) {
	if (!isInvited(nick))
		invitedUsers.push_back(nick);
}

void Channel::removeInvitedUser(const std::string &nick) {
	for (size_t i = 0; i < invitedUsers.size(); ++i) {
		if (invitedUsers[i] == nick) {
			invitedUsers.erase(invitedUsers.begin() + i);
			break;
		}
	}
}

    bool Channel::isInviteOnly() {
        return inviteOnly;
    }

    // Можно добавить сеттер для inviteOnly, если нужно менять позже
    void Channel::setInviteOnly(bool value) {
        inviteOnly = value;
    }
