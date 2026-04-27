#ifndef BOT_HPP
#define BOT_HPP
#include "../server/Server.hpp"
#include <iostream>
#include <string>


class Bot : public Client {
    public:
        Bot(int fd);
        ~Bot();
        void connectToServer();
};


#endif