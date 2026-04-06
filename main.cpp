#include "Server.hpp"


int main(int ac, char **av){
    if (ac != 3)
    {
        std::cerr << "Usage : ./server <port> <password>" << std::endl;
        return 1;
    }
    int port = std::atoi(av[1]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Invalid port!" << std::endl;
        return 1;
    }
    std::string password  = av[2];
    Server server(port, password);
    if (server.init() != 0)
        return 1;
    server.run();
    return 0;
}