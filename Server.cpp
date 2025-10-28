#include "Server.hpp"

Server::Server(int port, std::string pass) {
	this->port = port;
	this->pass = pass;
	std::cout << "OK! port = " << this->port << ", pass = " << this->pass << ".\n";
}

std::vector<std::string> Server::split(const std::string& res, const std::string& delim) {
	std::vector<std::string> vec;
	std::string s = res;
	size_t i;
	while ((i = s.find(delim)) != std::string::npos) {
		std::string token = s.substr(0, i);
		if (!token.empty())
			vec.push_back(token);
		s = s.substr(i + delim.length());
	}
	if (!s.empty())
		vec.push_back(s);
	return (vec);
}

void Server::setIfRegistered(User &user, int clientFd) {
	if (!user.getPass().empty() && !user.getNickname().empty() && !user.getUsername().empty()) {
		user.setRegistered(true);
		std::string nick = user.getNickname();
		std::string welcome = ":server 001 " + nick + " :Welcome to the IRC server\r\n";
		welcome += ":server 002 " + nick + " :Your host is server, running MyIRC\r\n";
		welcome += ":server 003 " + nick + " :This server was created just now\r\n";
		send(clientFd, welcome.c_str(), welcome.size(), 0);
	}
}

std::string Server::trim(const std::string& str) {
	size_t start = 0;
	size_t end = str.size();
	while (start < end && (str[start] == ' ' || str[start] == '\r' || str[start] == '\n' || str[start] == '\t'))
		++start;
	while (end > start && (str[end - 1] == ' ' || str[end - 1] == '\r' || str[end - 1] == '\n' || str[end - 1] == '\t'))
		--end;
	return str.substr(start, end - start);
}

void Server::sendJoinReply(User &user, Channel *channel) {
	std::string channelName = channel->getName();
	std::string joinMsg = ":" + user.getNickname() + "!" + user.getUsername() + "@localhost JOIN :" + channelName + "\r\n";
	const std::vector<User*> &clients = channel->getClients();
	for (size_t i = 0; i < clients.size(); ++i) {
		int fd = clients[i]->getFd();
		send(fd, joinMsg.c_str(), joinMsg.size(), 0);
	}
	
	std::string names = ":server 353 " + user.getNickname() + " = " + channelName + " :";
	for (size_t i = 0; i < clients.size(); ++i) {
		names += clients[i]->getNickname();
		if (i + 1 < clients.size())
			names += " ";
	}
	names += "\r\n";
	send(user.getFd(), names.c_str(), names.size(), 0);
	std::string endNames = ":server 366 " + user.getNickname() + " " + channelName + " :End of NAMES list\r\n";
	send(user.getFd(), endNames.c_str(), endNames.size(), 0);
}

User* Server::getUserByNickname(const std::string& nickname) {
    for (std::map<int, User>::iterator it = allConnections.begin(); it != allConnections.end(); ++it) {
        if (it->second.getNickname() == nickname)
            return &(it->second);
    }
    return NULL;
}


int handleModeCommand(User &user, const std::string &command, std::set<Channel*> &channels)
{
    // Проверка регистрации
    if (!user.getRegistered()) {
        std::string err = ":server 451 " + user.getNickname() + " :You have not registered\r\n";
        send(user.getFd(), err.c_str(), err.size(), 0);
        return -8;
    }

    // Разбор команды: MODE #канал +флаги [параметр]
    std::istringstream iss(command.substr(5));
    std::string channelName, modeFlags, param;
    iss >> channelName >> modeFlags;
    std::getline(iss, param);
    param = Channel::trimTopic(param);

    if (channelName.empty() || modeFlags.empty()) {
        std::string err = ":server 461 " + user.getNickname() + " MODE :Not enough parameters\r\n";
        send(user.getFd(), err.c_str(), err.size(), 0);
        return -1;
    }

    Channel* channel = NULL;
    std::set<Channel*>::iterator it;
    for (it = channels.begin(); it != channels.end(); ++it) {
        if ((*it)->getName() == channelName) {
            channel = *it;
            break;
        }
    }

    if (!channel) {
        std::string err = ":server 403 " + user.getNickname() + " " + channelName + " :No such channel\r\n";
        send(user.getFd(), err.c_str(), err.size(), 0);
        return -1;
    }

    if (!channel->isOperator(&user)) {
        std::string err = ":server 482 " + user.getNickname() + " " + channelName + " :You're not channel operator\r\n";
        send(user.getFd(), err.c_str(), err.size(), 0);
        return -1;
    }

    bool add = true;
	std::istringstream paramStream(param);
    for (size_t i = 0; i < modeFlags.size(); ++i) {
        char c = modeFlags[i];
        if (c == '+') add = true;
        else if (c == '-') add = false;
        else if (c == 'i') channel->setInviteOnly(add);
        else if (c == 't') channel->setTopicRestricted(add);
		else if (c == 'k') {
			std::string key;
			paramStream >> key;
			if (add && key.empty()) {
				std::string err = ":server 461 " + user.getNickname() + " MODE :Not enough parameters\r\n";
				send(user.getFd(), err.c_str(), err.size(), 0);
				continue; // можно заменить на return -1, если хочешь остановить выполнение
			}

			if (add) channel->setPassword(key);
			else channel->removePassword();
		}
		else if (c == 'o') {
			std::string targetNick;
			paramStream >> targetNick;
			if (targetNick.empty()) continue;

			User* targetUser = channel->getUserByNick(targetNick);
			if (!targetUser || !channel->hasClient(targetUser)) {
				std::string err = ":server 441 " + user.getNickname() + " " + targetNick + " " + channel->getName() + " :They aren't on that channel\r\n";
				send(user.getFd(), err.c_str(), err.size(), 0);
				continue;
			}

			if (add) channel->addOperator(targetUser);
			else channel->removeOperator(targetUser);
		}
        else if (c == 'l') {
            if (add) {
                std::istringstream iss_limit(param);
                size_t limit = 0;
                if (!(iss_limit >> limit)) channel->removeUserLimit();
                else channel->setUserLimit(limit);
            } else {
                channel->removeUserLimit();
            }
        }
    }

    // Broadcast сообщения о смене режима
    std::string modeMsg = ":" + user.getNickname() + "!" + user.getUsername() +
                          "@localhost MODE " + channelName + " " + modeFlags;
    // if (!param.empty()) modeMsg += " " + param;
    modeMsg += "\r\n";

    channel->broadcastMessage(modeMsg);

    std::cout << "[MODE] " << user.getNickname() << " changed mode for "
              << channelName << " to: " << modeFlags << " param: " << param << std::endl;

    return 0; // успех
}

int Server::processCommand(User &user, std::string command) {
	if (command.compare(0, 4, "PING") == 0) {
		std::string nums = command.substr(5);
		if (!nums.empty() && nums[0] == ':')
			nums.erase(0, 1);
		std::string response = "PONG :" + nums + "\r\n";
		send(user.getFd(), response.c_str(), response.size(), 0);
		std::cout << "Responded to PING with PONG\n";
		return 10;
	}
	if (user.getRegistered() == false) {
		if (command.rfind("CAP", 0) == 0) {
			if (command.find("LS") != std::string::npos) {
				std::string caps = "multi-prefix sasl";
				std::string reply = ":server CAP * LS :" + caps + "\r\n";
				send(user.getFd(), reply.c_str(), reply.size(), 0);
				return 8;
			}
			if (command.find("REQ") != std::string::npos) {
				size_t pos = command.find(':');
				std::string req = (pos != std::string::npos) ? trim(command.substr(pos + 1)) : "";
				std::vector<std::string> accepted;
				std::istringstream iss(req);
				std::string c;
				while (iss >> c) {
					if (c == "multi-prefix" || c == "sasl") accepted.push_back(c);
				}
				std::string ack = ":server CAP * ACK :";
				for (size_t k = 0; k < accepted.size(); ++k) {
					if (k) ack += " ";
					ack += accepted[k];
				}
				ack += "\r\n";
				send(user.getFd(), ack.c_str(), ack.size(), 0);
				return 8;
			}
			if (command.find("END") != std::string::npos) {
				return 9;
			}
		}
		if (command.substr(0, 4) == "PASS") {
			std::string pass = trim(command.substr(5));
			if (!pass.empty() && pass[pass.size() - 1] == '\n')
				pass.erase(pass.size() - 1);
			if (!pass.empty() && pass[pass.size() - 1] == '\r')
				pass.erase(pass.size() - 1);
			if (pass.empty()) {
				std::cerr << "Your password is empty, try again!" << std::endl;
				return -1;
			}
			if (pass != this->pass) {
				std::cerr << "Your password is wrong, try again!" << std::endl;
				return -1;
			}
			user.setPass(pass);
			std::cout << "OK pass" << std::endl;
			setIfRegistered(user, user.getFd());
			if (user.getRegistered() == true)
				std::cout << "User " << user.getUsername() << " is registered now!" << std::endl;
			return 1;
		}
		if (command.compare(0, 5, "NICK ") == 0) {
			std::string nick = trim(command.substr(5));
			if (nick.empty()) {
        	                std::cerr << "Your nickname is empty, try again!" << std::endl;
				return -2;
        	        }
			user.setNickname(nick);
			std::cout << "OK nick" << std::endl;
			setIfRegistered(user, user.getFd());
			if (user.getRegistered() == true)
				std::cout << "User " << user.getUsername() << " is registered now!" << std::endl;
			return 2;
		}
		if (command.compare(0, 5, "USER ") == 0) {
			std::string res = command.substr(5);
			if (!res.empty() && res[res.size() - 1] == '\n')
				res.erase(res.size() - 1);
			if (!res.empty() && res[res.size() - 1] == '\r')
				res.erase(res.size() - 1);
			if (res.empty()) {
				std::cerr << "Your USER command is empty, try again!" << std::endl;
				return -3;
			}
			size_t colon = res.find(':');
	       		std::string before, realname;
			if (colon != std::string::npos) {
				before = res.substr(0, colon);
				realname = res.substr(colon + 1);
			}
			else {
				std::cerr << "Your USER command is missing a realname" << std::endl;
				return -3;
			}
			while (!before.empty() && before[before.size() - 1] == ' ')
				before.erase(before.size() - 1, 1);
			while (!before.empty() && before[0] == ' ')
				before.erase(0, 1);
			std::vector<std::string> vec = split(before, " ");
			if (vec.size() != 3) {
				std::cerr << "Your user settings are wrong, try again!" << std::endl;
				return -3;
			}
			user.setUsername(trim(vec[0]));
			user.setRealname(trim(realname));
			std::cout << "OK user" << std::endl;
			setIfRegistered(user, user.getFd());
			if (user.getRegistered() == true)
				std::cout << "User " << user.getUsername() << " is registered now!" << std::endl;
			return 3;
		}
		std::cerr << command << std::endl;
		std::cerr << "Your command is wrong, try again!" << std::endl;
	}
	else if(user.getRegistered() == true) {
			if (command.compare(0, 8, "PRIVMSG ") == 0) {
				if (user.getRegistered() == false) {
					std::cerr << "You are not registered, you cannot send any messages" << std::endl;
					return -4;
				}
				std::string msg = trim(command.substr(8));
				if (!msg.empty() && msg[msg.size() - 1] == '\n')
					msg.erase(msg.size() - 1);
				if (!msg.empty() && msg[msg.size() - 1] == '\r')
					msg.erase(msg.size() - 1);
				if (msg.empty()) {
					std::cerr << "Your message is empty, try again!" << std::endl;
					return -4;
				}
				size_t colon = msg.find(':');
				std::string sendTo, text;
				if (colon != std::string::npos) {
					sendTo = msg.substr(0, colon);
					text = msg.substr(colon + 1);
				}
				else {
					std::cerr << "Your PRIVMSG command is missing something" << std::endl;
					return -4;
				}
				if (!sendTo.empty() && !text.empty()) {
					return 4;
				}
				else {
					std::cerr << "Your PRIVMSG command is missing message or destination" << std::endl;
					return -4;
				}
				std::cerr << "Your command is wrong, try again!" << std::endl;
				return -4;
			}
			else if (command.compare(0, 7, "NOTICE ") == 0) {
				if (user.getRegistered() == false) {
					std::cerr << "You are not registered, you cannot send any NOTICE messages" << std::endl;
					return -5;
				}
				std::string msg = trim(command.substr(7));
				if (!msg.empty() && msg[msg.size() - 1] == '\n')
					msg.erase(msg.size() - 1);
				if (!msg.empty() && msg[msg.size() - 1] == '\r')
					msg.erase(msg.size() - 1);
				if (msg.empty()) {
					std::cerr << "Your NOTICE is empty, try again!" << std::endl;
					return -5;
				}
				size_t colon = msg.find(':');
				std::string sendTo, text;
				if (colon != std::string::npos) {
					sendTo = msg.substr(0, colon - 1);
					text = msg.substr(colon + 1);
				}
				else {
					std::cerr << "Your NOTICE command is missing something" << std::endl;
					return -5;
				}
				if (!sendTo.empty() && !text.empty()) {
					return 5;
				}
				else {
					std::cerr << "Your NOTICE command is missing message or destination" << std::endl;
					return -5;
				}
				std::cerr << "Your command is wrong, try again!" << std::endl;
				return -5;
			}
			else if(command.compare(0, 5, "JOIN ") == 0) {
				if (user.getRegistered() == false) {
					std::cerr << "You are not registered, you cannot JOIN a chat" << std::endl;
                                        return -6;
                                }
                                std::string chat = command.substr(5);
				if (chat.find('#') == std::string::npos) {
					std::string errMsg = ":server 475 " + user.getNickname() + " " + chat + " :Cannot join channel (+k) - bad key\r\n";
					send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
					return -6;
				}
				if (!chat.empty() && chat[chat.size() - 1] == '\n')
					chat.erase(chat.size() - 1);
				if (!chat.empty() && chat[chat.size() - 1] == '\r')
					chat.erase(chat.size() - 1);
				if (chat.empty()) {
					std::string errMsg = ":server 475 " + user.getNickname() + " " + chat + " :Cannot join channel (+k) - bad key\r\n";
					send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
					return -6;
				}
				size_t space = chat.find(' ');
                                std::string chatName, password;
				std::vector<std::string> channelsNew, keys;
                                if (space != std::string::npos) {
                                        chatName = chat.substr(0, space);
                                        password = chat.substr(space + 1);
					channelsNew = split(chatName, ",");
					keys = split(password, ",");
                                }
				else {
					channelsNew = split(chat, ",");
				}
				if (!channelsNew.empty()) {
					for (size_t j = 0; j < channelsNew.size(); j++) {
						bool flag = false;
						std::string key = (j < keys.size()) ? keys[j] : "";
						if (channelsNew[j].find('#') == std::string::npos) {
							std::string errMsg = ":server 475 " + user.getNickname() + " " + channelsNew[j] + " :Cannot join channel (+k) - bad key\r\n";
							send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
							return -6;
						}
						for (std::set<Channel *>::iterator it = channels.begin(); it != channels.end(); ++it) {
							if ((*it)->getName() == channelsNew[j] && (*it)->getPassword().empty()) {
								  // ✅ Проверка invite-only
							if ((*it)->getName() == channelsNew[j]) {
									// ✅ Проверка invite-only
									if ((*it)->isInviteOnly() && !(*it)->isInvited(user.getNickname())) {
										std::string errMsg = ":server 473 " + user.getNickname() + " " + (*it)->getName() + " :Cannot join channel (+i)\r\n";
										send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);

										std::cerr << "[JOIN] User " << user.getNickname() 
												<< " tried to join invite-only channel " 
												<< (*it)->getName() << " without invite." << std::endl;

										// Пользователь не добавляется в канал
										flag = true;
										return -6;
									}
									// Если проверка прошла — добавляем пользователя
									(*it)->addClient(&user);
									sendJoinReply(user, *it);
									flag = true;
									continue;
							}}
							else if (key != "" && (*it)->getName() == channelsNew[j] && (*it)->getPassword() == key) {
								(*it)->addClient(&user);
								flag = true;
								sendJoinReply(user, *it);
								continue;
							}
							else if (key != "" && (*it)->getName() == channelsNew[j] && (*it)->getPassword() != key) {
								std::cerr << "Your password for the chat is wrong, try again!" << std::endl;
								flag = true;
								std::string errMsg = ":server 475 " + user.getNickname() + " " + channelsNew[j] + " :Cannot join channel (+k) - bad key\r\n";
								send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
								return -6;
							}
							else if ((*it)->getName() == channelsNew[j] && !(*it)->getPassword().empty()) {
								std::cerr << "You forgot a password!" << std::endl;
								flag = true;
								std::string errMsg = ":server 475 " + user.getNickname() + " " + channelsNew[j] + " :Cannot join channel (+k) - bad key\r\n";
								send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
								return -6;
                                                	}
						}
						Channel* newChannel;
						if (!flag) {
							if (key != "") {
								newChannel = new Channel(&user, channelsNew[j], key);
							}
							else
								newChannel = new Channel(&user, channelsNew[j]);
							this->channels.insert(newChannel);
							sendJoinReply(user, newChannel);
						}
					}
					return 6;
				}
				std::string errMsg = ":server 475 " + user.getNickname() + " " + chat + " :Cannot join channel (+k) - bad key\r\n";
				send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
				return -6;
			}
			else if (command.compare(0, 5, "KICK ") == 0) {
				if (user.getRegistered() == false) {
					std::cerr << "You are not registered, you cannot KICK a chat" << std::endl;
					std::string err = ":server 451 " + user.getNickname() + " :You have not registered\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -7;
				}
				std::string usersAndChats = trim(command.substr(5));
				if (usersAndChats.find('#') == std::string::npos) {
					std::cerr << "Your chat name doesn't have a hash character, try again!" << std::endl;
					std::string err = ":server 403 " + user.getNickname() + " " + usersAndChats + " :No such channel\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -7;
				}
				if (!usersAndChats.empty() && usersAndChats[usersAndChats.size() - 1] == '\n')
					usersAndChats.erase(usersAndChats.size() - 1);
				if (!usersAndChats.empty() && usersAndChats[usersAndChats.size() - 1] == '\r')
					usersAndChats.erase(usersAndChats.size() - 1);
				if (usersAndChats.empty()) {
					std::cerr << "Your chat name is empty, try again!" << std::endl;
					std::string err = ":server 403 " + user.getNickname() + " " + usersAndChats + " :No such channel\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -7;
				}
				size_t space = usersAndChats.find_first_of(' ');
				std::vector<std::string> chats, chatUsers;
				std::string msg;
				if (space != std::string::npos) {
					std::string tmp_c = usersAndChats.substr(0, space);
					usersAndChats = usersAndChats.substr(space + 1);
					space = usersAndChats.find_first_of(' ');
					if (space != std::string::npos) {
						std::string tmp_u = usersAndChats.substr(0, space);
						chats = split(tmp_c, ",");
						chatUsers = split(tmp_u, ",");
						msg = usersAndChats.substr(space + 1);
					}
					else {
						std::cerr << "No such channel or user!" << std::endl;
						std::string err = ":server 403 " + user.getNickname() + " " + usersAndChats + " :No such channel\r\n";
						send(user.getFd(), err.c_str(), err.size(), 0);
						return -7;
					}
				}
				else {
					std::cerr << "Your chats or names doesn't exist, try again!" << std::endl;
					std::string err = ":server 403 " + user.getNickname() + " " + usersAndChats + " :No such channel\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -7;
				}
				for (size_t j = 0; j < chats.size(); j++) {
					bool flag = false;
					std::string deleteUser = (j < chatUsers.size()) ? trim(chatUsers[j]) : trim(chatUsers[chatUsers.size() - 1]);
					for (std::set<Channel *>::iterator it = channels.begin(); it != channels.end(); ++it) {
						if ((*it)->isOperator(&user) && (*it)->getName() == trim(chats[j]) && (*it)->hasClient(deleteUser)) {
							(*it)->removeClient(deleteUser);
							 std::string kickMsg = ":" + user.getNickname() + "!" + user.getUsername() + "@server KICK " + (*it)->getName() + " " + deleteUser + " " + msg + "\r\n";
							for (std::map<int, User>::iterator it2 = allConnections.begin(); it2 != allConnections.end(); it2++) {
								if (it2->second.getNickname() == deleteUser) {
									send(it2->first, kickMsg.c_str(), kickMsg.size(), 0);
									(*it)->sendNotification(allConnections, kickMsg);
									flag = true;
								}
							}
						}
					}
					if (!flag) {
						 std::string errMsg = ":server 482 " + user.getNickname() + " " + trim(chats[j]) + " :You're not channel operator\r\n";
						 send(user.getFd(), errMsg.c_str(), errMsg.size(), 0);
					}
				}
			}
			else if (command.compare(0, 7, "INVITE ") == 0) {
                if (user.getRegistered() == false) {
                    std::cerr << "You are not registered, you cannot KICK a chat" << std::endl;
                    std::string err = ":server 451 " + user.getNickname() + " :You have not registered\r\n";
                    send(user.getFd(), err.c_str(), err.size(), 0);
                    return -8;
                }
                std::istringstream iss(command.substr(7));
                std::string targetNick, channelName;
                iss >> targetNick >> channelName;
				//проверяем аргументы
                if (targetNick.empty() || channelName.empty()){
                    std::cerr << "INVITE: Not enough parameters" << std::endl;
                    std::string err = ":server 461 " + user.getNickname() + " INVITE :Not enough parameters\r\n";
                    send(user.getFd(), err.c_str(), err.size(), 0);
					return -8;
                }
				// Найти пользователя по нику
				User* targetUser = getUserByNickname(targetNick);
				if (!targetUser) {
					std::string err = "401 " + user.getNickname() + " " + targetNick + " :No such nick\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -1;
				}
				//проверяем, есть ли канал
                Channel* channel = NULL;
				for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
					if ((*it)->getName() == channelName) {
						channel = *it;
						break;
					}
				}
				if (!channel) {
					std::cerr << "INVITE: No such channel " << channelName << std::endl;
					std::string err = ":server 403 " + user.getNickname() + " " + channelName + " :No such channel\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -8;
				}
				//проверяем, является ли юзер оператором канала
				if (!channel->isOperator(&user)){
					std::cerr << "INVITE: " << user.getNickname() << " is not channel operator" << std::endl;
					std::string err = ":server 482 " + user.getNickname() + " " + channelName + " :You're not channel operator\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -8;
				}
				//проверка есть ли пользователь уже в этом канале
				if (channel->hasClient(targetNick)){
					std::cerr << "INVITE: user already in channel" << std::endl;
					std::string err = ":server 443 " + user.getNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -8;
				}
				//отправляет приглашение тому, кто отправляет приглашение
				std::string confirmMsg = ":server 341 " + user.getNickname() + " " + targetNick + " " + channelName + "\r\n";
				send(user.getFd(), confirmMsg.c_str(), confirmMsg.size(), 0);

				//отправляет приглашение целевому пользователю
				// Сформировать и отправить приглашение
				std::string inviteMsg = ":" + user.getNickname() + "!" + user.getUsername() +
										"@localhost INVITE " + targetNick + " :" + channelName + "\r\n";

				send(targetUser->getFd(), inviteMsg.c_str(), inviteMsg.size(), 0);
				
				// // ✅ Добавляем пользователя в список приглашённых
				// channel->addInvitedUser(targetNick);
				
				std::cout << "[INVITE] " << user.getNickname() << " invited " 
						<< targetNick << " to " << channelName << std::endl;
				return 7;
			}
			else if (command.compare(0, 6, "TOPIC ") == 0) {
				if (user.getRegistered() == false) {
                    std::cerr << "You are not registered, you cannot set a TOPIC" << std::endl;
                    std::string err = ":server 451 " + user.getNickname() + " :You have not registered\r\n";
                    send(user.getFd(), err.c_str(), err.size(), 0);
                    return -8;
				}
                std::istringstream iss(command.substr(5));
				std::string channelName, topic;
				iss >> channelName;

				//собрать остаток строки в топик, если он есть
				std::getline(iss, topic);
				// Чистим topic
				topic = Channel::trimTopic(topic);


				// Проверка аргумента
				if (channelName.empty()) {
					std::string err = ":server 461 " + user.getNickname() + " TOPIC :Not enough parameters\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -1;
				}

				Channel* channel = NULL;
				for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it)
				{
					if ((*it)->getName() == channelName) {
						channel = *it;
						break;
					}
				}
				if (!channel) {
					std::string err = ":server 403 " + user.getNickname() + " " + channelName + " :No such channel\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -1;
				}

				//проверяем, пустая ли тема в запросе, если да, возвращаем текущую тему канала
				if (topic.empty()){
					if (channel->getTopic().empty()){
						std::string msg = ":server 331 " + user.getNickname() + " " + channelName + " :No topic is set\r\n";
						send(user.getFd(), msg.c_str(), msg.size(), 0);
					} else {
						std::string msg = ":server 332 " + user.getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n";
						send(user.getFd(), msg.c_str(), msg.size(), 0);
					}
					return 0;
				}

				// Проверка прав — оператор ли пользователь
				if (channel->getTopicRestricted() && !channel->isOperator(&user)) {
					std::string err = ":server 482 " + user.getNickname() + " " + channelName + " :You're not channel operator\r\n";
					send(user.getFd(), err.c_str(), err.size(), 0);
					return -1;
				}

				if (topic.size() > 512) {
    			    topic = topic.substr(0, 512);
    			}

				channel->setTopic(topic);
				std::string topicMsg = ":" + user.getNickname() + "!" + user.getUsername() +
						"@localhost TOPIC " + channelName + " :" + topic + "\r\n";
				channel->broadcastMessage(topicMsg);

				std::cout << "[TOPIC] " << user.getNickname() << " set topic for "
						<< channelName << " to: " << topic << std::endl;
				return 8;
			}
			else if (command.compare(0, 5, "MODE ") == 0)
			{
				return handleModeCommand(user, command, channels);
			}
			else if (command.compare(0, 4, "QUIT") == 0) {
				std::string reason;
				size_t pos = command.find(':');
				if (pos != std::string::npos)
					reason = command.substr(pos + 1);
				else
					reason = "Client Quit";

				std::string quitMsg = ":" + user.getNickname() + "!" + user.getUsername() +
									"@localhost QUIT :" + reason + "\r\n";

				for (std::set<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
					if ((*it)->hasClient(&user))
						(*it)->sendToUsers(quitMsg, allConnections, user);
				}

				removeUserFromAllChannels(&user);
				close(user.getFd());
				allConnections.erase(user.getFd());

				std::cout << "[QUIT] " << user.getNickname() 
						<< " disconnected (" << reason << ")" << std::endl;
				return 9;
			}
            return -1;
		}
	return 0;
}

void Server::sendMsg(User &user, std::string command, int listeningSocket, int countOfChars) {
	int sizeOfCommand = (countOfChars == 4) ? 8 : 7;
	sockaddr_in peer;
	socklen_t peerSize = sizeof(peer);
	getpeername(user.getFd(), (sockaddr*)&peer, &peerSize);
	char peerIP[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &peer.sin_addr, peerIP, INET_ADDRSTRLEN);
	std::string msg = command.substr(sizeOfCommand);
	size_t colon = msg.find(':');
	std::string sendTo = msg.substr(0, colon - 1);
	std::vector<std::string> sendToUsers = split(sendTo, ",");
	std::string text = msg.substr(colon);
	std::string message = ":" + user.getNickname() + "!" + user.getUsername() 
		+ "@" +  std::string(peerIP);
	for (size_t n = 0; n < sendToUsers.size(); n++) {
		if (sendToUsers[n][0] != '#') {
			bool found = false;
			for (size_t j = 0; j < fdClients.size(); j++) {
				std::map<int, User>::iterator it = this->allConnections.find(fdClients[j].fd);
				if (it != this->allConnections.end()) {
					User &toUser = it->second;
					if (fdClients[j].fd != listeningSocket &&
							fdClients[j].fd != user.getFd() &&
							toUser.getNickname() == sendToUsers[n]) {
						std::string tmp = message + " " + command.substr(0, sizeOfCommand) + sendToUsers[n] + " " + text + "\r\n";
						send(fdClients[j].fd, tmp.c_str(), tmp.size(), 0);
						found = true;
						break;
					}
				}
				else if (sendToUsers[n] == "server") {
					std::string tmp = message + " " + command.substr(0, sizeOfCommand) + sendToUsers[n] + " " + text;
					std::cout << tmp << std::endl;
					found = true;
				}
			}
			if (!found && sizeOfCommand != 7) {
				std::string err = ":server 401 " + user.getNickname() + " " + sendToUsers[n] + " :No such nick/channel\r\n";
				send(user.getFd(), err.c_str(), err.size(), 0);
			}
		}
		else {
			for (std::set<Channel *>::iterator it = channels.begin(); it != channels.end(); ++it) {
				if ((*it)->getName() == sendToUsers[n]) {
					if (!(*it)->hasClient(&user)) {
						std::string err = ":server 442 " + user.getNickname() + " " + sendToUsers[n] + " :You're not on that channel\r\n";
						send(user.getFd(), err.c_str(), err.size(), 0);
						continue;
					}
					std::string tmp = message + " " + command.substr(0, sizeOfCommand) + sendToUsers[n] + " " + text + "\r\n";
					(*it)->sendToUsers(tmp, this->allConnections, user);
				}
			}
		}
	}
}

void Server::removeUserFromAllChannels(User *user) {
	std::set<Channel *>::iterator it = channels.begin();
	while (it != channels.end()) {
		Channel *channel = *it;
		channel->removeClient(user);
		channel->removeOperator(user);
		if (channel->isEmpty()) {
			delete channel;
			std::set<Channel *>::iterator tmp = it;
			++it;
			channels.erase(tmp);
		}
		else {
			++it;
		}
	}
}


void Server::startServer() {
	int listeningSocket = socket(AF_INET, SOCK_STREAM, 0);
	// Сокет (socket) — это как виртуальный разъём для соединения двух программ по сети.
	// Listening socket — это главный «приёмник звонков». Его задача — только принимать новые подключения.
	// AF_INET → IPv4 || AF_INET6 → IPv6 || AF_UNIX → локальное взаимодействие процессов на одной машине
	// SOCK_STREAM → потоковsize_t colon = res.find(':');                                                                                                        std::string before, realname;                                                                                                        if (colon != std::string::npos) {                                                                                                            before = res.substr(0, colon);                                                                                                       realname = res.substr(colon + 1);                                                                                            }                                                                                                                                    else {                                                                                                                                       std::cerr << "Your USER command is missing a realname" << std::endl;                                                                 return;                                                                                                                      }ый сокет (TCP, надёжный, с установкой соединения) || SOCK_DGRAM → датаграммный сокет (UDP, быстрый, но без гарантии доставки) || SOCK_RAW → «сырой» доступ к сетевым протоколам (для особых задач, например утилита ping)

	//1. создал listening socket — «главный приёмник for new connections».
	if (listeningSocket == -1) {
		throw std::runtime_error("Something wrong with listening socket (create listening socket)");
	}
	int opt = 1;
	if (setsockopt(listeningSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		throw std::runtime_error("setsockopt(SO_REUSEADDR) failed");
	}
	struct sockaddr_in hint;
	// isa_family_t    sin_family; // семейство адресов (обычно AF_INET)
	// in_port_t      sin_port;   // порт (нужно хранить в сетевом порядке байтов)
	// struct in_addr sin_addr;   // IP-адрес (структура)
	// unsigned char  sin_zero[8];// не используется, для выравнивания

	hint.sin_family = AF_INET;
	hint.sin_port = htons(this->port);
	// We have to use this function because on our computers represent numbers in little-endian format(младший байт идёт первым)
	// But networks represent in big-endian format(старший байт идёт первым).
	// 6667 = 0x1A0B
	// В little-endian (например, x86): 0B 1A
	// В big-endian (сетевой порядок): 1A 0B

	//inet_pton(AF_INET, "127.0.0.1", &hint.sin_addr);
	// AF_INET Указывает, что мы работаем с IPv4-адресом. Для IPv6 было бы AF_INET6.
	// "127.0.0.1" Читаемый IP-адрес в виде строки (так называемая presentation form). То есть, сервер будет принимать подключения с IP, который у него есть: с 127.0.0.1 (локальный хост)
	// &sa.sin_addr Указатель на поле sin_addr в структуре sockaddr_in. inet_pton запишет туда IP в сетевом порядке байтов (big-endian, 32-битное число).
	//2. подготовила структуру с адресом и портом, к которым будет привязан сокет.

	hint.sin_addr.s_addr = INADDR_ANY;  // ДЛЯ ПОДКЛЮЧЕНИЯ С РАЗНЫХ КОМПОВ, слушаем все интерфейсы. пРОВЕРЯЕМ ПО КОМАНДЕ С ДРУГОГО КОМПА /connect <IP_твоего_компьютера> 6667 mypassword

	if (bind(listeningSocket, (struct sockaddr*)&hint, sizeof(hint)) == -1) {
		throw std::runtime_error("Error: Something wrong with listening socket (bind the socket to a IP/port)");
	}
	// 3. bind «привязывает» сокет к IP и порту, которые ты указала в hint. После bind сокет знает, на каком IP и порту принимать подключения.

	if (listen(listeningSocket, SOMAXCONN) == -1) {
		throw std::runtime_error("Error: Something wrong with listening socket (listening failed)");
	}
	// 4. listen - устанавливает сокет в режим прослушивания
	// Для каждого входящего подключения потом используется accept(), чтобы создать новый сокет для общения с конкретным клиентом.
	// backlog определяет максимальную длину очереди неподтверждённых подключений. Если клиентов приходит больше, чем позволяет очередь, новые подключения будут отвергнуты.
	// backlog = SOMAXCONN, В Linux есть макрос SOMAXCONN, но в glibc он обычно определён как 128 и используется как «рекомендованное» значение
	pollfd pfd;
	// pollfd — это структура из <poll.h> (или <sys/poll.h> на Linux), которая описывает один файл/сокет для функции poll().
	// Структура pollfd примерно такая:
	// struct pollfd {
	// int fd;         // Дескриптор файла или сокета
	// short events;   // Какие события мы хотим отслеживать (например, POLLIN)
	// short revents;  // Какие события реально произошли (заполняется poll())
	// }; Таким образом, fdClients — это вектор всех сокетов, за которыми мы следим с помощью poll().
	pfd.fd = listeningSocket; //Она будет представлять один сокет — в данном случае наш listening socket.
	pfd.events = POLLIN; // events — это битовое поле, где указываются события, за которыми мы хотим следить. POLLIN — это макрос, который означает: «на этом сокете есть данные для чтения». Для listening socket это означает: «к нам подключился клиент, готов accept()».
	fdClients.push_back(pfd); //Мы добавляем pfd в fdClients, чтобы наш listening socket стал отслеживаться функцией poll() вместе с другими клиентами.
	char buffer[4096];
	while (true) {
		//Функция poll() из <poll.h> используется для неблокирующего или блокирующего ожидания событий на нескольких сокетах одновременно.
		int activity = poll(fdClients.data(), fdClients.size(), -1); // -1 → бесконечно ждать, пока какое-то событие не произойдёт.
		if (activity == -1) {
			throw std::runtime_error("Error: poll error");
		}
		for (size_t i = 0; i < fdClients.size(); i++) {
			if (fdClients[i].revents & POLLIN) { // после вызова функции poll() система сама кладет в переменную revets то, что произошло во время вызова функции. В events мы показываем системе то, что хотим, чтобы произошло. Используем & потому что нас интересует конкретно POLLIN. Напимер: revents = POLLIN | POLLERR = 0001 | 0010 = 0011; 0011 (revents) & 0001 (POLLIN) == 0001 → true
				if (fdClients[i].fd == listeningSocket) {
					sockaddr_in client;
					socklen_t clientSize = sizeof(client);
					int clientSocket = accept(listeningSocket, (sockaddr*)&client, &clientSize); //Создаёт новый сокет для общения с конкретным клиентом.
					if (clientSocket == -1) {
						std::cerr << "Error: Accept failed" << std::endl;
						continue;
					}
					pollfd newfd;
					newfd.fd = clientSocket;
					newfd.events = POLLIN;
					fdClients.push_back(newfd);
					char clientIP[INET_ADDRSTRLEN];
					inet_ntop(AF_INET, &client.sin_addr, clientIP, INET_ADDRSTRLEN);
					std::cout << "New client connected: " << clientIP << ":" << ntohs(client.sin_port) << std::endl;
					this->allConnections.insert(std::make_pair(clientSocket, User(clientSocket)));
				}
				else {
					memset(buffer, 0, 4096);
					int bytesRecv = recv(fdClients[i].fd, buffer, 4096, 0); //читает данные из сокета
					if (bytesRecv <= 0) {
						// std::cout << "Client disconnected" << std::endl;
						// close(fdClients[i].fd);
						// std::map<int, User>::iterator it = this->allConnections.find(fdClients[i].fd);
						// if (it != this->allConnections.end()) {
						// 	User &user = it->second;
						// 	removeUserFromAllChannels(&user);
						// }
						// this->allConnections.erase(fdClients[i].fd);
						// fdClients.erase(fdClients.begin() + i);
						// i--;
						std::map<int, User>::iterator it = allConnections.find(fdClients[i].fd);
						if (it != allConnections.end()) {
							User &user = it->second;

							// 1. Локальный лог отключения
							std::cout << "[DISCONNECT] " << user.getNickname()
									<< " (fd=" << fdClients[i].fd << ") disconnected" << std::endl;

							// 2. Отправляем QUIT-сообщение всем пользователям на каналах
							std::string quitMsg = ":" + user.getNickname() + "!" + user.getUsername() +
												"@localhost QUIT :Connection closed\r\n";
							for (std::set<Channel*>::iterator ch = channels.begin(); ch != channels.end(); ++ch) {
								if ((*ch)->hasClient(&user))
									(*ch)->sendToUsers(quitMsg, allConnections, user);
							}

							// 3. Удаляем пользователя из всех каналов
							removeUserFromAllChannels(&user);

							// 4. Закрываем соединение и удаляем из структуры клиентов
							close(fdClients[i].fd);
							allConnections.erase(fdClients[i].fd);
						}

						// 5. Убираем fd из fdClients
						fdClients.erase(fdClients.begin() + i);
						--i; // чтобы не пропустить следующий элемент
					}
					else {
						std::map<int, User>::iterator it = this->allConnections.find(fdClients[i].fd);
						if (it != this->allConnections.end()) {
							User &user = it->second;
							// добавляем все полученные данные в буфер пользователя
							user.getBuffer().append(buffer, bytesRecv);
							 // проверяем, есть ли полная команда (оканчивается на \r\n)
						 	size_t pos;
							while ((pos = user.getBuffer().find("\r\n")) != std::string::npos) {
								 std::string command = user.getBuffer().substr(0, pos);
								 user.getBuffer().erase(0, pos + 2);
								 int countOfChars = processCommand(user, command);
								 if (countOfChars == 4 || countOfChars == 5) {
									 sendMsg(user, command, listeningSocket, countOfChars);
								 }
							}
						}
					}
				}
			}
		}
	}
	// Listening socket - не передаёт данные, единственная задача — ждать новых подключений.
	// Client socket cоздаётся автоматически, когда сервер вызывает accept(). У него уже есть конкретный клиентский адрес и порт (например, клиент подключился с 127.0.0.1:54321). Через этот сокет сервер может общаться (читать и писать данные) именно с этим клиентом.
	// Удаляем всех пользователей из каналов
	for (std::map<int, User>::iterator it = allConnections.begin(); it != allConnections.end(); ++it) {
    	removeUserFromAllChannels(&it->second);
	}
	for (size_t i = 0; i < fdClients.size(); i++) {
		close(fdClients[i].fd);
	}
	close(listeningSocket);
	listeningSocket = -1;
	for (std::set<Channel *>::iterator it = channels.begin(); it != channels.end(); ++it) {
		delete *it;
	}
	channels.clear();
}

Server::Server() {
}

Server::~Server() {
}

Server::Server(Server const &other) {
	this->port = other.port;
	this->pass = other.pass;
}

Server& Server::operator=(Server const &other) {
	if (&other != this) {
		this->port = other.port;
		this->pass = other.pass;
	}
	return *this;
}
