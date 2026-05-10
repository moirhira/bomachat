#include "Bot.hpp"


int main(int ac, char **av)
{
    if (ac != 4)
    {
        std::cerr << "Error:\nUsage ./bot <address> <port> <password>" << std::endl;
        return 1;
    }
    std::string address = av[1];
    std::string password = av[3];

    int port = std::atoi(av[2]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Invalid port!" << std::endl;
        return 1;
    }
    Bot bot(port, address);

    std::string nick = "bot";
    std::string user = "botUser";
    std::string real = "boma";

    if (!bot.init(nick, user, real, password))
        return 1;

    if (!bot.connectToServer(password))
        return 1;
    
    bot.run();

    return 0;
}