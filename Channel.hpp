#ifndef CHANNEL_HPP
#define CHANNEL_HPP
#include "Client.hpp"
#include <string>
#include <iostream>
#include <vector>


class Channel {
    private:
        std::string _name;
        std::vector<Client*> _members;
        std::vector<Client*> _operators;
        std::string _pass;
        std::string _topic;
        int _usrLimit;
        bool _inviteOnly;
        bool _topicRestricted;
        std::vector<Client*> _inviteLst;

    public:
        Channel(std::string& name,  Client* client);
        ~Channel();

        std::string getName() const;
        std::string getPass() const;
        std::string getTopic() const;
        int getUserlimit() const;

        void setPass(std::string pass);
        void setTopic(std::string topic);
        void setUsrlimit(int limit);
        void setInviteOnly(bool v);
        void setTopicRestricted(bool v);


        void addToInviteList(Client* client);
        void addMember(Client* client);
        bool isMember(Client* client);
        bool isOperator(Client* client);
        bool isInvited(Client* client);
        bool isInviteOnly();
        bool isTopicRestricted();


};


#endif