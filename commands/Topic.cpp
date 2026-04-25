#include "ServerCommands.hpp"
#include "Server.hpp"

void handleTopic(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "TOPIC");
        return;
    }
    if (params[0][0] != '#')
    {
        sendReply(client, 476, "Channel name should start with #", params[0]);
        return;
    }
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == params[0])
        {
            if (!channels[i].isMember(client))
            {
                sendReply(client, 442, "You are not on that channel", params[0]);
                return;
            }
            if (params.size() == 1)
            {
                if (!channels[i].getTopic().empty())
                    sendReply(client, 332, channels[i].getTopic(), params[0]);
                else
                    sendReply(client, 331, "No topic is set", params[0]);
                return;
            }
            if (channels[i].isTopicRestricted() && !channels[i].isOperator(client))
            {
                sendReply(client, 482, "You are not allowed to change the topic", params[0]);
                return;
            }
            channels[i].setTopic(params[1]);
            std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost TOPIC " + params[0] + " :" + params[1] + "\r\n";
            std::vector<Client *> members = channels[i].getMembers();
            for (size_t j = 0; j < channels[i].getMembers().size(); j++)
            {
                members[j]->sendMessage(topicMsg);
            }
            return;
        }
    }
    sendReply(client, 403, "Channel doesn't exist", params[0]);
}