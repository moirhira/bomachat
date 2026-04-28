#include "Bot.hpp"


int main(int ac, char **av)
{
    if (ac != 3)
    {
        std::cerr << "Error:\nUsage ./bot <address> <port>" << std::endl;
        return 1;
    }
    std::string address = av[1];
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
    bot.init(nick, user, real);
    bot.connectToServer();

}