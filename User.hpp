#ifndef USER_HPP
#define USER_HPP

#include <string>

class User {
	private:
		int fd;               // файловый дескриптор сокета
		std::string nickname; // NICK
		std::string username; // USER
		std::string realname; // USER с "Real Name"
		std::string hostname; // можешь хранить ip/host клиента
		std::string pass;
		std::string buffer;
                User();
		bool registered;      // зарегистрирован ли (получил и NICK, и USER)
		bool invited;
		bool param;
	public:
		User(User const &other);
                User& operator=(User const &other);
		User(int fd);
		~User();
		std::string getNickname() const;
		std::string getUsername() const;
		std::string getRealname() const;
		std::string getHostname() const;
		std::string getPass() const;
		int getFd() const;
		bool getRegistered() const;
		bool getInvited()const;
		bool getParam() const;
		void setPass(std::string const &pass);
		void setNickname(std::string const &nickname);
		void setUsername(std::string const &username);
		void setRealname(std::string const &realname);
		void setHostname(std::string const &hostname);
		void setRegistered(bool registered);
		void setInvited(bool invited);
		void setParam(bool param);
		std::string &getBuffer();
};

#endif
