#ifndef SERVER_HPP
#define SERVER_HPP
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <string>
#include <unistd.h>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include "Client.hpp"
#include <sstream>
#include <iomanip>
#include "Channel.hpp"

struct command
{
    std::string command;
    std::vector<std::string> params;
};


class Server {
    private:
        int _sockfd;
        std::vector<Client*> _clients;
        int _port;
        std::string _password;
        struct pollfd _fds[1024];
        int _nfds;
        std::vector<Channel> _channels;
    public:
        Server(int port, std::string password);
        ~Server();

        int init();
        void run();
        void acceptClient();
        void handelClient(int& i);

        Client* getClientById(int id);
        command parseCommand(std::string cmdLine);
        void handelCommand(command cmd, Client* client);
};

#endif