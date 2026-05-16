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
#include <fcntl.h>
#include <errno.h>
#include <signal.h>

struct command
{
    std::string command;
    std::vector<std::string> params;
};


class Server 
{
    private:
		int _port;
        std::string _password;
        int _sockfd;
		std::vector<pollfd> pfds;
        std::vector<Client*> _clients;
        std::vector<Channel> _channels;
    public:

        Server(int port, std::string password);
        ~Server();

        void CreateServerSocket();
		void run();
		void Accept_client();
		void Respond_to_client(int i);
		void removeClient(int fd);
		void Receive_input(int i);
		void disconnectClient(int i);

        Client* getClientById(int id);
        command parseCommand(std::string cmdLine);
        void handelCommand(command cmd, Client* client, int& i);
		void parse_cmd(Client *client, int i, int curFd);

		void set_port(int p);
		void set_password(std::string &pass);
		int get_port() const;
		int get_server_fd();
		const std::string &get_password();
};

#endif