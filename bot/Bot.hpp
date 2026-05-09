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
        
        command parseCommand(std::string cmdLine);
        void    handelCommand(command cmd);
        void    handlePrivmsg(const command &cmd);

        void sendMessge(const std::string &msg);
        bool flushSendBuffer(struct pollfd &pfd);
        bool handleRecv(struct pollfd &pfd);
        bool doRegistration();


    public:
        Bot(int port, std::string address);
        ~Bot();

        int             getServerPort();
        std::string	    getServerAdr();

        int init(std::string nickName, std::string userName, std::string realName);

        int connectToServer(std::string password);
        void run();

        

};
#endif