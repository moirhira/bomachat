#include "ServerCommands.hpp"
#include "Server.hpp"
#include <cctype>


void handleNick(Client *client, std::vector<std::string> &params, std::vector<Client *> &clients, std::vector<Channel> &channels)
{
    if (params.size() < 1)
    {
        sendReply(client, 431, "No nickname given", "NICK");
        return;
    }
    if (!client->isAuth())
    {
        sendReply(client, 451, "You have not registered", "NICK");
        return;
    }
    std::string newNick = params[0];
    if (newNick.empty())
    {
        sendReply(client, 431, "No nickname given", "NICK");
        return;
    }

    if (isdigit(newNick[0]) || newNick[0] == '-')
    {
        sendReply(client, 432, "Nickname cannot start with \"-\" or Number", "NICK");
        return;
    }
    for (size_t i = 0; i < newNick.size(); i++)
    {
        if (!isalnum(newNick[i]) && newNick[i] != '-' && newNick[i] != '_' && newNick[i] != '[' && newNick[i] != ']' && newNick[i] != '\\' && newNick[i] != '^' && newNick[i] != '{' && newNick[i] != '}' && newNick[i] != '|')
        {
            sendReply(client, 432, "Nickname can only contain letters, digits, and - _ ' [ ] \\ ^ { } |", "NICK");
            return;
        }
    }
    if (newNick == "bot")
    {
        sendReply(client, 432, "Nickname is reserved", "NICK");
        return;
    }
    if (newNick.size() > 9)
    {
        sendReply(client, 432, "Nickname too long", "NICK");
        return;
    }
    for (size_t j = 0; j < clients.size(); j++)
    {
        if (clients[j]->getFd() != client->getFd() && clients[j]->getNickname() == newNick)
        {
            sendReply(client, 433, "Nickname is already in use", newNick);
            return;
        }
    }
    bool wasReg = client->isReg();
    std::string oldNick = client->getNickname();

    client->setNickname(newNick);

    if (!client->getUsername().empty())
        client->setRegistered(true);

    if (client->isReg() && !wasReg)
    {
        sendWelcome(client);
    }
    else if (wasReg)
    {
        std::string nickMsg = ":" + oldNick + "!" + client->getUsername() + "@localhost NICK :" + newNick + "\r\n";
        for (size_t j = 0; j < channels.size(); j++)
        {
            channels[j].brodcastMessage(nickMsg, client);
        }
        client->sendMessage(nickMsg);
    }
}