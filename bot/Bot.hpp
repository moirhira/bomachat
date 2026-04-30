#ifndef BOT_HPP
#define BOT_HPP
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <vector>

struct command
{
    std::string command;
    std::vector<std::string> params;
    std::string prefix;
};



class Bot{
    private:
        int _fd;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        std::string _buffer;
        int _sPort;
        std::string _sAddress;

        void handelCommand(command cmd);
        command parseCommand(std::string cmdLine);
        void handlePrivmsg(const command &cmd);
        void handleKick();
        void handleError();
        void handleJoin();
    public:
        Bot(int port, std::string address);
        ~Bot();

        void init(std::string nickName, std::string userName, std::string realName);

        int getServerPort();
        std::string  getServerAdr();
        void connectToServer(std::string password);
        void run();

        

};


#endif