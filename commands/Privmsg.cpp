#include "ServerCommands.hpp"
#include "Server.hpp"


void handlePrivmsg(Client *client, std::vector<std::string> params, std::vector<Client *> &clients, std::vector<Channel> &channels)
{
    if (params.size() == 0)
    {
        sendReply(client, 411, "No recipient given (PRIVMSG)", "PRIVMSG");
        return;
    }
    if (params.size() == 1)
    {
        sendReply(client, 412, "No text to send", "PRIVMSG");
        return;
    }
    std::string target = params[0];
    std::string msg = params[1];
    if (target[0] == '#')
    {
        for (size_t i = 0; i < channels.size(); i++)
        {
            if (channels[i].getName() == target)
            {
                if (!channels[i].isMember(client))
                {
                    sendReply(client, 442, "You are not on that channel", target);
                    return;
                }

                std::vector<Client *> members = channels[i].getMembers();
                for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                {
                    if (client->getFd() != members[j]->getFd())
                    {
                        std::string prvMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PRIVMSG " + target + " :" + msg + "\r\n";
                        members[j]->sendMessage(prvMsg);
                    }
                }
                return;
            }
        }
        sendReply(client, 403, "Channel doesn't exist", target);
        return;
    }

    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i]->getNickname() == target)
        {
            std::string prvMsg = ":" + client->getNickname() + "!" +
                                 client->getUsername() + "@localhost PRIVMSG " +
                                 target + " :" + msg + "\r\n";
            clients[i]->sendMessage(prvMsg);
            return;
        }
    }
    sendReply(client, 401, "No such nick/channel", target);
}