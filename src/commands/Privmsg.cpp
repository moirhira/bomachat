#include "ServerCommands.hpp"
#include "Server.hpp"


static Channel *findChannel(std::vector<Channel> &channels, const std::string &name)
{
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == name)
            return &channels[i];
    }
    return NULL;
}

void handlePrivmsg(Client *client, std::vector<std::string> params,
                    std::vector<Client *> &clients, std::vector<Channel> &channels)
{
    if (params.empty())
        return sendReply(client, "411", "No recipient given (PRIVMSG)", "PRIVMSG");
    if (params.size() < 2)
        return sendReply(client, "412", "No text to send", "PRIVMSG");

    std::string target = params[0];
    std::string fullMsg = ":" + client->getNickname() + "!"
                        + client->getUsername() + "@localhost PRIVMSG "
                        + target + " :" + params[1] + "\r\n";

    if (target[0] == '#')
    {
        Channel *channel = findChannel(channels, target);
        if (!channel)
            return sendReply(client, "403", "No such channel", target);
        if (!channel->isMember(client))
            return sendReply(client, "442", "You're not on that channel", target);

        std::vector<Client *> members = channel->getMembers();
        for (size_t i = 0; i < members.size(); i++)
        {
            if (members[i]->getFd() != client->getFd())
                members[i]->sendMessage(fullMsg);
        }
        return;
    }

    Client *dest = findClientByNick(clients, target, NULL);
    if (!dest)
        return sendReply(client, "401", "No such nick/channel", target);
    dest->sendMessage(fullMsg);
}