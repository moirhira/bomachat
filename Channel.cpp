#include "Channel.hpp"


Channel::Channel(std::string& name,  Client* client) 
    : _name(name) , _usrLimit(0) , _inviteOnly(false), _topicRestricted(false)
{
    _operators.push_back(client);
    addMember(client);
}
Channel::~Channel() {};

std::string Channel::getTopic() const {
    return _topic;
}
int Channel::getUserlimit() const {
    return _usrLimit;
}

std::string Channel::getName() const {
    return _name;
}

std::string Channel::getPass() const {
    return _pass;
}

std::vector<Client*> Channel::getMembers() const {
    return _members;
}

 std::vector<Client*> Channel::getOperators() const {
    return _operators;
 }

void Channel::setPass(std::string pass) {
    _pass = pass;
}
void Channel::setTopic(std::string topic) {
    _topic = topic;
}
void Channel::setUsrlimit(int limit) {
    _usrLimit = limit;
}


void Channel::addMember(Client* client) {
    _members.push_back(client);
}

void Channel::addOperator(Client* client) {
    _operators.push_back(client);
}

void Channel::removeOperator(Client* client) {
    for (size_t i = 0; i < _operators.size(); i++)
    {
        if (_operators[i] == client)
        {
            _operators.erase(_operators.begin() + i);
            break;
        }
    }
}

bool Channel::isMember(Client* client) {
   for (size_t i = 0; i < _members.size(); i++)
    {
        if (_members[i] == client)
            return true;
    }
   return false;
}


bool Channel::isOperator(Client* client) {
    for (size_t i = 0; i < _operators.size(); i++)
    {
        if (_operators[i] == client)
            return true;
    }
   return false;
}


void Channel::setInviteOnly(bool v) {
    _inviteOnly = v;
}

void Channel::setTopicRestricted(bool v) {
    _topicRestricted = v;
}

void Channel::addToInviteList(Client* client) {
    _inviteLst.push_back(client);
}

bool Channel::isInvited(Client* client) {
    for (size_t i = 0; i < _inviteLst.size(); i++)
    {
        if (_inviteLst[i] == client)
            return true;
    }
   return false;
}

bool Channel::isInviteOnly() {
    return _inviteOnly;
}

bool Channel::isTopicRestricted() {
    return _topicRestricted;
}