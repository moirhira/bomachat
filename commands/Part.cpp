#include "ServerCommands.hpp"
#include "Server.hpp"

void handlePart(Client *client, std::vector<std::string> params, std::vector<Channel> &channels) {
    (void)channels;
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "PART");
        return;
    }
    std::string reason;
    if (params.size() > 1)
        reason = params[1];
    std::vector<std::string> channelList = splitCommaList(params[0]);
    for (size_t i = 0; i < channelList.size(); i++)
    {
        std::string channelName = channelList[i];
        if (channelName[0] != '#')
        {
            sendReply(client, 476, "Channel name should start with #", channelName);
            return;
        }
        bool found = false;
        for (size_t j = 0; j < channels.size(); j++)
        {
            if (channelName == channels[j].getName())
            {
                found = true;
                if (!channels[j].isMember(client))
                {
                    sendReply(client, 476, "You are not on that channel",channelName);
                    break;
                }
                channels[j].removeClientEverywhere(client);
                std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
                if (!reason.empty())
                    partMsg += " :" + reason;
                partMsg += "\r\n";
                channels[j].brodcastMessage(partMsg, client);
                client->sendMessage(partMsg);
                if (channels[j].isEmpty())
                {
                    channels.erase(channels.begin() + j);
                }
                break;
            }
        }
        if (!found)
        {
            sendReply(client, 403, "Channel doesn't exist", channelName);
            return;
        }
    }
}