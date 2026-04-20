#include "ServerCommands.hpp"
#include "Server.hpp"

void sendReply(Client *client, int errorCode, std::string errorMsg, std::string cmd)
{
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    std::ostringstream oss;
    oss << std::setw(3) << std::setfill('0') << errorCode;
    std::string reply = ":server " + oss.str() + " " + nick + " " + cmd + " :" + errorMsg + "\r\n";
    client->sendMessage(reply);
}

static void sendWelcome(Client *client)
{
    sendReply(client, 001, "Welcome to the IRC server " + client->getNickname(), "");
    sendReply(client, 002, "Your host is ircserv", "");
    sendReply(client, 003, "This server was created today", "");
    sendReply(client, 004, "ircserv", "");
}

bool isPreRegistrationCommand(const std::string &cmd)
{
    return (cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "CAP" || cmd == "PING" || cmd == "PONG");
}

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
        return;
    }
    client->setAuthenticated(true);
}

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
    if (newNick.size() > 9)
    {
        sendReply(client, 432, "Nickname too long", "NICK");
        return;
    }
    for (size_t j = 0; j < clients.size(); j++)
    {
        if (clients[j]->getFd() != client->getFd() && clients[j]->getNickname() == newNick)
        {
            sendReply(client, 433, "nickname already taken by another client", "NICK");
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

void handleUser(Client *client, const std::vector<std::string> params)
{
    if (!client->isAuth())
    {
        sendReply(client, 451, "You have not registered", "USER");
        return;
    }
    if (client->isReg())
    {
        sendReply(client, 462, "You are already registred!", "USER");
        return;
    }
    if (params.size() < 4)
    {
        sendReply(client, 461, "Not enough parameters", "USER");
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
    while (start < list.size())
    {
        size_t commaPos = list.find(',');
        size_t len;
        if (commaPos == std::string::npos)
            len = list.size() - start;
        else
            len = commaPos - start;
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
    if (!isValidChannelName(params[0]))
    {
        sendReply(client, 476, "Channel name is invalid", "JOIN");
        return;
    }
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (params[0] == channels[i].getName())
        {
            if (channels[i].isMember(client))
                return;
            if (channels[i].isInviteOnly() && !channels[i].isInvited(client))
            {
                sendReply(client, 473, "You are not invited to this channel", params[0]);
                return;
            }
            if (!channels[i].getPass().empty())
            {
                std::string pswd;
                if (params.size() > 1)
                    pswd = params[1];
                else
                    pswd = "";
                if (channels[i].getPass() != pswd)
                {
                    sendReply(client, 475, "Invalid channel password", params[0]);
                    return;
                }
            }
            if (channels[i].getUserlimit() != 0 && channels[i].getUserlimit() <= (int)channels[i].getMembers().size())
            {
                sendReply(client, 471, "Channel is full", params[0]);
                return;
            }
            channels[i].addMember(client);
            sendJoinReply(client, channels[i]);
            return;
        }
    }
    channels.push_back(Channel(params[0], client));
    sendJoinReply(client, channels.back());
}

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
                    sendReply(client, 442, "You are not on that channel", "PRIVMSG");
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
        sendReply(client, 403, "Channel doesn't exist", "PRIVMSG");
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
    sendReply(client, 401, target + " :No such nick/channel", "PRIVMSG");
}

void handleTopic(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "TOPIC");
        return;
    }
    if (params[0][0] != '#')
    {
        sendReply(client, 476, "Channel name should start with #", "TOPIC");
        return;
    }
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == params[0])
        {
            if (!channels[i].isMember(client))
            {
                sendReply(client, 442, "You are not on that channel", "TOPIC");
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
                sendReply(client, 482, "You are not allowed to change the topic", "TOPIC");
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
    sendReply(client, 403, "Channel doesn't exist", "TOPIC");
}

void handleMode(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 2)
    {
        sendReply(client, 461, "Not enough parameters", "MODE");
        return;
    }
    if (params[0][0] != '#')
    {
        if (params[0] == client->getNickname())
            return;
        sendReply(client, 476, "Channel name should start with #", "MODE");
        return;
    }
    if (!client->isReg())
    {
        sendReply(client, 451, "You have not registered", "MODE");
        return;
    }
    if (params[1].size() < 2 || (params[1][0] != '+' && params[1][0] != '-'))
    {
        sendReply(client, 472, "Unknown mode flag", "MODE");
        return;
    }
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == params[0])
        {
            if (!channels[i].isOperator(client))
            {
                sendReply(client, 482, "You're not channel operator", "MODE");
                return;
            }
            char sign = params[1][0];
            char flag = params[1][1];
            switch (flag)
            {
            case 'i':
            {
                if (sign == '+')
                    channels[i].setInviteOnly(true);
                else
                    channels[i].setInviteOnly(false);
                std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + params[0] + " " + params[1] + "\r\n";
                std::vector<Client *> members = channels[i].getMembers();
                for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                {
                    members[j]->sendMessage(msgReply);
                }
                break;
            }
            case 'o':
            {
                if (params.size() < 3)
                {
                    sendReply(client, 461, "Not enough parameters", "MODE");
                    return;
                }
                Client *targetClient = NULL;
                for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                {
                    if (channels[i].getMembers()[j]->getNickname() == params[2])
                    {
                        targetClient = channels[i].getMembers()[j];
                        break;
                    }
                }
                if (!targetClient)
                {
                    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
                    std::string reply = ":server 441 " + nick + " " + params[2] + " " + params[0] + " :They aren't on that channel\r\n";
                    client->sendMessage(reply);
                    return; 
                }
                if (sign == '+')
                {
                    if (!channels[i].isOperator(targetClient))
                    {
                        channels[i].addOperator(targetClient);
                    }
                    std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + params[0] + " " + params[1] + " " + params[2] + "\r\n";
                    std::vector<Client *> members = channels[i].getMembers();
                    for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                    {
                        members[j]->sendMessage(msgReply);
                    }
                    break;
                }
                else
                {
                    channels[i].removeOperator(targetClient);
                    std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + params[0] + " " + params[1] + " " + params[2] + "\r\n";
                    std::vector<Client *> members = channels[i].getMembers();
                    for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                    {
                        members[j]->sendMessage(msgReply);
                    }
                    break;
                }
            }
            case 'l':
            {
                if (sign == '-')
                {
                    channels[i].setUsrlimit(0);
                }
                else
                {
                    if (params.size() < 3)
                    {
                        sendReply(client, 461, "Not enough parameters", "MODE");
                        return;
                    }
                    if (!isdigit(static_cast<unsigned char>(params[2][0])))
                    {
                        sendReply(client, 461, "Not enough parameters", "MODE");
                        return;
                    }
                    for (size_t i = 0; i < params[2].size(); i++)
                    {
                        if (!isdigit(params[2][i]))
                        {
                            sendReply(client, 460, "User limit should be a number", "MODE");
                            return;
                        }
                    }
                    long newUserLimit = strtol(params[2].c_str(), NULL, 10);
                    if (newUserLimit <= 0 || newUserLimit >= 1024)
                    {
                        sendReply(client, 460, "User limit should be 1 >=  <= 1024", "MODE");
                        return;
                    }
                    channels[i].setUsrlimit(static_cast<int>(newUserLimit));
                }
                std::string modeArg = (sign == '+' ? " " + params[2] : "");
                std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + params[0] + " " + params[1] + modeArg + "\r\n";
                std::vector<Client *> members = channels[i].getMembers();
                for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                {
                    members[j]->sendMessage(msgReply);
                }
                break;
            }
            case 'k':
            {
                if (sign == '-')
                {
                    channels[i].setPass("");
                }
                else
                {
                    if (params.size() < 3)
                    {
                        sendReply(client, 461, "Not enough parameters", "MODE");
                        return;
                    }
                    channels[i].setPass(params[2]);
                }
                std::string modeArg = (sign == '+' ? " " + params[2] : "");
                std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + params[0] + " " + params[1] + modeArg + "\r\n";
                std::vector<Client *> members = channels[i].getMembers();
                for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                {
                    members[j]->sendMessage(msgReply);
                }
                break;
            }
            case 't':
            {
                if (sign == '+')
                    channels[i].setTopicRestricted(true);
                else
                    channels[i].setTopicRestricted(false);
                std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost MODE " + params[0] + " " + params[1] + "\r\n";
                std::vector<Client *> members = channels[i].getMembers();
                for (size_t j = 0; j < channels[i].getMembers().size(); j++)
                {
                    members[j]->sendMessage(msgReply);
                }
                break;
            }
            default:
                sendReply(client, 472, std::string("Unknown mode flag ") + flag, "MODE");
                return;
            }
            return;
        }
    }
    sendReply(client, 403, "Channel doesn't exist", "MODE");
    return;
}

void handleKick(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 2)
    {
        sendReply(client, 461, "Not enough parameters", "KICK");
        return;
    }
    if (params[0][0] != '#')
    {
        sendReply(client, 476, "Channel name should start with #", "KICK");
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
                    std::vector<Client *> members = channel.getMembers();
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
                    sendReply(client, 441, "Target client not on this channel", "KICK");
                    return;
                }
                else
                {
                    sendReply(client, 482, "You're not channel operator", "KICK");
                    return;
                }
            }
            else
            {
                sendReply(client, 442, "You are not on that channel", "KICK");
                return;
            }
        }
    }
    sendReply(client, 403, "Channel doesn't exist", "KICK");
    return;
}

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
        sendReply(client, 401, "No such nick", "INVITE");
        return;
    }

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() == targetChannel)
        {
            if (!channels[i].isMember(client))
            {
                sendReply(client, 442, "You are not on that channel", "INVITE");
                return;
            }

            if (!channels[i].isOperator(client))
            {
                sendReply(client, 482, "You're not channel operator", "INVITE");
                return;
            }

            if (channels[i].isMember(target))
            {
                sendReply(client, 443, targetClientNick + " :is already on channel", "INVITE");
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
    sendReply(client, 403, "No such channel", "INVITE");
}
