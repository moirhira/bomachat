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

class Bot{
    private:
        int _fd;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        int _sPort;
        std::string _sAddress;
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