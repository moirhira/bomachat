#include "ServerCommands.hpp"
#include "Server.hpp"

void handleKick(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 2)
    {
        sendReply(client, "461", "Not enough parameters", "KICK");
        return;
    }
    if (params[0][0] != '#')
    {
        sendReply(client, "476", "Channel name should start with #", params[0]);
        return;
    }
    std::string targetChannel = params[0];
    std::string targetUser = params[1];
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == targetChannel)
        {
            Channel &channel = channels[i];
            if (channel.isMember(client))
            {
                if (channel.isOperator(client))
                {
                    const std::vector<Client *> &members = channel.getMembers();
                    for (size_t m = 0; m < members.size(); m++)
                    {
                        if (members[m]->getNickname() == targetUser)
                        {
                            std::string msgReply;
                            if (params.size() == 2)
                                msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost KICK " + params[0] + " " + params[1] + "\r\n";
                            else
                                msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost KICK " + params[0] + " " + params[1] + " :" + params[2] + "\r\n";

                            for (size_t j = 0; j < members.size(); j++)
                            {
                                members[j]->sendMessage(msgReply);
                            }
                            channel.removeOperator(members[m]);
                            channel.removeMember(members[m]);
                            channel.removeFromInviteList(members[m]);
                            return;
                        }
                    }
                    sendReply(client, "441", "They aren't on that channel", targetUser + " " + targetChannel);
                    return;
                }
                else
                {
                    sendReply(client, "482", "You're not channel operator", targetChannel);
                    return;
                }
            }
            else
            {
                sendReply(client, "442", "You are not on that channel", targetChannel);
                return;
            }
        }
    }
    sendReply(client, "403", "Channel doesn't exist", targetChannel);
    return;
}