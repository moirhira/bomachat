#include "ServerCommands.hpp"
#include "Server.hpp"


static void sendJoinReply(Client *client, Channel &channel)
{
    std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost JOIN " + channel.getName() + "\r\n";
    client->sendMessage(joinMsg);

    std::string nick = client->getNickname();
    std::string channnelName = channel.getName();

    if (!channel.getTopic().empty())
    {
        std::string topicReply = ":server 332 " + nick + " " + channnelName + " :" + channel.getTopic() + "\r\n";
        client->sendMessage(topicReply);
    }
    else
    {
        std::string noTopic = ":server 331 " + nick + " " + channnelName + " :NO topic is set\r\n";
        client->sendMessage(noTopic);
    }

    std::string namesList = ":server 353 " + nick + " = " + channnelName + " :";
    std::vector<Client *> members = channel.getMembers();
    for (size_t i = 0; i < members.size(); i++)
    {
        if (channel.isOperator(members[i]))
            namesList += "@";
        namesList += members[i]->getNickname();
        if (i + 1 < members.size())
            namesList += " ";
    }

    namesList += "\r\n";
    client->sendMessage(namesList);

    std::string endNames = ":server 366 " + nick + " " + channnelName + " :End of /NAMES list\r\n";
    client->sendMessage(endNames);
}

bool isValidChannelName(const std::string& name)
{
    if (name.empty() || name[0] != '#' || name.size() == 1)
        return false;
    for (size_t i = 1;  i < name.size(); i++)
    {
        char c = name[i];

        if (c == ' ' || c == ',' || c < 32 || c == 127)
            return false;
    }
    return true;
}

static std::vector<std::string> splitCommaList(const std::string& list)
{
    std::vector<std::string> items;
    size_t start = 0;
    while (start <= list.size())
    {
        size_t commaPos = list.find(',', start);
        size_t len = (commaPos == std::string::npos) ? list.size() - start : commaPos - start;
        items.push_back(list.substr(start, len));
        if (commaPos == std::string::npos)
            break;
        start = commaPos + 1;
    }
    return items;
}

void handleJoin(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "JOIN");
        return;
    }
    if (!client->isReg())
    {
        sendReply(client, 451, "You have not registered", "JOIN");
        return;
    }
    std::vector<std::string> channelList = splitCommaList(params[0]);
    std::vector<std::string> keyList;
    if (params.size() > 1)
        keyList = splitCommaList(params[1]);
    
    for (size_t i = 0; i < channelList.size(); i++)
    {
        std::string channelName = channelList[i];
        std::string key;
        if (i < keyList.size())
            key = keyList[i];
        else
            key = "";

        if (!isValidChannelName(channelName))
        {
            sendReply(client, 476, "Channel name is invalid", channelName);
            continue;
        }
        bool found = false;
        for (size_t j = 0; j < channels.size(); j++)
        {
            if (channelName == channels[j].getName())
            {
                found = true;
                if (channels[j].isMember(client))
                    break;
                if (channels[j].isInviteOnly() && !channels[j].isInvited(client))
                {
                    sendReply(client, 473, "You are not invited to this channel", channelName);
                    break;
                }
                if (!channels[j].getPass().empty())
                {
                    if (channels[j].getPass() != key)
                    {
                        sendReply(client, 475, "Invalid channel password", channelName);
                        break;
                    }
                }
                if (channels[j].getUserlimit() > 0 && channels[j].getUserlimit() <= static_cast<int>(channels[j].getMembers().size()))
                {
                    sendReply(client, 471, "Channel is full", channelName);
                    break;
                }
                channels[j].addMember(client);
                sendJoinReply(client, channels[j]);
                break;
            }
        }
        if (!found)
        {
            channels.push_back(Channel(channelName, client));
            sendJoinReply(client, channels.back());
        }
    }
}