#include "Client.hpp"

Client::Client() : _fd(-1), _authenticated(false), _registred(false)
{}

Client::Client(int fd) : _fd(fd), _authenticated(false), _registred(false)
{}

Client::~Client() {}

int Client::getFd() const {
    return _fd;
}

std::string Client::getNickname() const {
    return _nickname;
}

std::string Client::getUsername() const {
    return _username;
}

std::string Client::getRealname() const { 
    return _realname;
}

bool Client::isAuth() {
    return _authenticated;
}

bool Client::isReg(){ 
    return _registred;
}

std::string& Client::getBuffer() {
    return const_cast<std::string&>(_buffer);
}



void Client::setNickname(std::string nickname){ 
    _nickname = nickname;
}

void Client::setUsername(std::string username) {
    _username = username;
}

void Client::setRealname(std::string realname) {
    _realname = realname;
}

void Client::setAuthenticated(bool value) {
    _authenticated = value;
}

void Client::setRegistered(bool value) {
    _registred = value;
}

void Client::appendToBuffer(std::string data){ 
    _buffer += data;
}

void Client::clearBuffer() {
    _buffer.clear();
}


void Client::sendMessage(const std::string& msg) {
    _outBuffer += msg;
}

std::string &Client::getOutBuffer() {
    return _outBuffer;
}

const std::string &Client::getOutBuffer() const {
    return _outBuffer;
}


bool Client::hasPendingOutput() const {
    return !_outBuffer.empty();
}

short Client::getClientEvents() const {
    short events = POLLIN;
    if (hasPendingOutput()) {
        events |= POLLOUT;
    }
    return events;
}