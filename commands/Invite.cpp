#include "ServerCommands.hpp"
#include "Server.hpp"


void handleInvite(Client *client, std::vector<std::string> params, std::vector<Channel> &channels, std::vector<Client *> &clients)
{
    if (params.size() < 2)
    {
        sendReply(client, 461, "Not enough parameters", "INVITE");
        return;
    }
    std::string targetClientNick = params[0];
    std::string targetChannel = params[1];

    Client *target = NULL;
    for (size_t j = 0; j < clients.size(); j++)
    {
        if (clients[j]->getNickname() == targetClientNick)
        {
            target = clients[j];
            break;
        }
    }
    if (!target)
    {
        sendReply(client, 401, "No such nick", targetClientNick);
        return;
    }

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == targetChannel)
        {
            if (!channels[i].isMember(client))
            {
                sendReply(client, 442, "You are not on that channel", targetChannel);
                return;
            }

            if (!channels[i].isOperator(client))
            {
                sendReply(client, 482, "You're not channel operator", targetChannel);
                return;
            }

            if (channels[i].isMember(target))
            {
                sendReply(client, 443, "is already on channel", targetClientNick + " " + targetChannel);
                return;
            }

            channels[i].addToInviteList(target);

            std::string senderMsg = ":server 341 " + client->getNickname() + " " + targetClientNick + " " + targetChannel + "\r\n";
            client->sendMessage(senderMsg);
            std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost INVITE " + targetClientNick + " " + targetChannel + "\r\n";
            target->sendMessage(msgReply);

            return;
        }
    }
    sendReply(client, 403, "No such channel", targetChannel);
}