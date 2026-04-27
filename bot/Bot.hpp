#ifndef BOT_HPP
#define BOT_HPP
#include "../server/Server.hpp"
#include <iostream>
#include <string>


class Bot : public Client {
    private:
        int _sPort;
        std::string _sAddress;
    public:
        Bot(int port, std::string address);
        ~Bot();

        int getServerPort();
        std::string  getServerAdr();
        void connectToServer();
};


#endif