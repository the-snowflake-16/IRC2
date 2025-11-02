#include "User.hpp"

std::string& User::getBuffer(){
	return this->buffer;
}

User::User(User const &other) {
	this->fd = other.fd;
	this->pass = other.pass;
	this->nickname = other.nickname;
	this->username = other.username;
	this->realname = other.realname;
	this->hostname = other.hostname;
	this->registered = other.registered;
}

User& User::operator=(User const &other) {
	if (this != &other) {
		this->fd = other.fd;
		this->pass = other.pass;
		this->nickname = other.nickname;
		this->username = other.username;
		this->realname = other.realname;
		this->hostname = other.hostname;
		this->registered = other.registered;
	}
	return *this;
}

User::User() {
}

User::User(int fd) {
	this->fd = fd;
	this->pass = "";
	this->nickname = "";
	this->username = "";
	this->realname = "";
	this->hostname = "";
	this->registered = false;
}

User::~User() {
}

void User::setPass(std::string const &pass) {
	this->pass = pass;
}

std::string User::getPass() const{
	return this->pass;
}

int User::getFd() const{
	return this->fd;
}

std::string User::getNickname() const {
	return this->nickname;
}

std::string User::getUsername() const {
	return this->username;
}

std::string User::getRealname() const {
	return this->realname;
}

std::string User::getHostname() const {
	return this->hostname;
}

bool User::getRegistered() const{
	return this->registered;
}

bool User::getInvited() const{
	return this->invited;
}

bool User::getParam() const{
	return this->param;
}

void User::setParam(bool param) {
	this->param = param;
}
void User::setInvited(bool invited) {
	this->invited = invited;
}
void User::setNickname(std::string const &nickname) {
	this->nickname = nickname;
}

void User::setUsername(std::string const &username) {
	this->username = username;
}

void User::setRealname(std::string const &realname) {
	this->realname = realname;
}

void User::setHostname(std::string const &hostname) {
	this->hostname = hostname;
}

void User::setRegistered(bool registered) {
	this->registered = registered;
}
