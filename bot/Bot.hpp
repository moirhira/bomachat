#ifndef BOT_HPP
#define BOT_HPP
#include "../server/Server.hpp"
#include "../server/Client.hpp"
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <iostream>

class Bot : public Client {
    private:
        int _sPort;
        std::string _sAddress;
    public:
        Bot(int port, std::string address);
        ~Bot();

        void init(std::string nickName, std::string userName, std::string realName);

        int getServerPort();
        std::string  getServerAdr();
        void connectToServer();
};


#endif