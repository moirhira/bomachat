#include "ServerCommands.hpp"
#include "Server.hpp"

void handlePass(Client *client, std::vector<std::string> params, std::string password)
{
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "PASS");
        return;
    }
    if (client->isAuth())
    {
        sendReply(client, 462, "You are already registred!", "PASS");
        return;
    }
    if (password != params[0])
    {
        sendReply(client, 464, "Worong password!", "PASS");
        close(client->getFd());
        return;
    }
    client->setAuthenticated(true);
}