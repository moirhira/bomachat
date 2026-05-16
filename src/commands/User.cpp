#include "ServerCommands.hpp"
#include "Server.hpp"


void handleUser(Client *client, const std::vector<std::string> params)
{
    if (!client->isAuth())
    {
        sendReply(client, "451", "You have not registered", "USER");
        return;
    }
    if (client->isReg())
    {
        sendReply(client, "462", "You are already registred!", "USER");
        return;
    }
    if (params.size() < 4)
    {
        sendReply(client, "461", "Not enough parameters", "USER");
        return;
    }
    client->setUsername(params[0]);
    client->setRealname(params[3]);
    if (!client->getNickname().empty())
    {
        client->setRegistered(true);
    }
    if (client->isReg())
        sendWelcome(client);
}