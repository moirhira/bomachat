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
#include <ctime>
#include <sstream>
#include <map>

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
        std::string _outBuffer;
        int _sPort;
        std::string _sAddress;
        std::map<std::string, std::time_t> _seenMap;
        
        void handelCommand(command cmd);
        command parseCommand(std::string cmdLine);
        void handlePrivmsg(const command &cmd);
    public:
        Bot(int port, std::string address);
        ~Bot();

        int init(std::string nickName, std::string userName, std::string realName);

        int getServerPort();
        std::string  getServerAdr();
        int connectToServer(std::string password);
        void run();


        void sendMessage(const std::string & msg);
        std::string &getOutBuffer();
        bool hasPendingOutput() const;
        // bool hasPendingMessages() const;
        short getClientEvents() const;

};


#endif