#ifndef SERVER_HPP
#define SERVER_HPP
#include <vector>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <strings>

class Server {
    private:
        int _sockfd;
        std::vector<Client> *_clients;
        int _port;
        std::string _password;
    public:
        Server(int port, std::string password);
        ~Server();

        int init();
        void run();
        void acceptClient();
        void handelClient();
};

#endif