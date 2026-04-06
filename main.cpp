#include "Server.hpp"


int main(int ac, char **av){
    if (ac != 3)
    {
        std::err << "Usage : ./server <port> <password>" << std::endl;
        return 1;
    }
    int port = std::atoi(av[1]);
    std::string password  = av[2];
    Server server(port, server);
    server.init();
    server.run();
    return 0;
}