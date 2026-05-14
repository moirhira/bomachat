#include "Bot.hpp"

volatile sig_atomic_t g_running = 1;

void sigintHandler(int signum)
{
    (void)signum;
    g_running = 0;
}
int main(int ac, char **av)
{
    signal(SIGINT, sigintHandler);
    if (ac != 4)
    {
        std::cerr << "Error:\nUsage ./bot <address> <port> <password>" << std::endl;
        return 1;
    }
    std::string address = av[1];

    int port = std::atoi(av[2]);
    if (port <= 0 || port > 65535)
    {
        std::cerr << "Invalid port!" << std::endl;
        return 1;
    }

    std::string password = av[3];
    if (password.empty())
	{
		std::cerr << "Password cannot be epmty!" << std::endl;
		return 1;
	}


    Bot bot(port, address);

    std::string nick = "bot";
    std::string user = "botUser";
    std::string real = "boma";

    if (!bot.init(nick, user, real, password))
        return 1;

    if (!bot.connectAsync())
    {
        std::cerr << "Failed to initiate connection" << std::endl;
        return 1;
    }

    bot.run();

    return 0;
}