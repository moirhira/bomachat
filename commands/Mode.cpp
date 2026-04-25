#include "ServerCommands.hpp"
#include "Server.hpp"

void handleMode(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
{
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "MODE");
        return;
    }
    std::string target = params[0];
    if (target[0] != '#')
    {
        if (target != client->getNickname())
        {
            sendReply(client, 502, "Can't change mode for other users", "MODE");
            return;
        }
        std::string reply = ":server 221 " + client->getNickname() + " +\r\n";
        client->sendMessage(reply);
        return;
    }

    if (params.size() == 1)
    {
        for (size_t i = 0; i < channels.size(); i++)
        {
            if (channels[i].getName() != target)
                continue;
            
            std::string modeStr = "+";
            std::string modeArgs;
            if (channels[i].isInviteOnly())
                modeStr += 'i';
            if (channels[i].isTopicRestricted())
                modeStr += 't';
            if (!channels[i].getPass().empty())
            {
                modeStr += 'k';
                modeArgs += " " + channels[i].getPass();
            }
            if (channels[i].getUserlimit() > 0)
            {
                modeStr += 'l';
                std::ostringstream oss;
                oss << channels[i].getUserlimit();
                modeArgs += " " + oss.str();
            }

            std::string reply = ":server 324 " + client->getNickname() + " " + target + " " + modeStr + modeArgs + "\r\n";
            client->sendMessage(reply);
            return;
        }
        sendReply(client, 403, "Channel doesn't exist", "MODE");
        return;
    }
    

    if (!client->isReg())
    {
        sendReply(client, 451, "You have not registered", "MODE");
        return;
    }

    if (params[1].empty() || (params[1][0] != '+' && params[1][0] != '-'))
    {
        sendReply(client, 472, "Unknown mode flag", "MODE");
        return;
    }

    for (size_t i = 0; i < channels.size(); i++)
    {
        if (channels[i].getName() != target)
            continue;

        if (!channels[i].isOperator(client))
        {
            sendReply(client, 482, "You're not channel operator", "MODE");
            return;
        }

        char sign = '+';
        size_t nextArgId = 2;
        std::string modeStr;
        std::string modeArgs;

        for  (size_t f = 0; f < params[1].size(); f++)
        {
            char flag = params[1][f];
            if (flag == '+' || flag == '-')
            {
                sign = flag;
                continue;
            }

            switch (flag)
            {
                case 'i':
                {
                    if (sign == '+')
                        channels[i].setInviteOnly(true);
                    else
                        channels[i].setInviteOnly(false);
                    modeStr += sign;
                    modeStr += 'i';
                    break;
                }
                case 'o':
                {
                    if (nextArgId >= params.size())
                    {
                        sendReply(client, 461, "Not enough parameters", "MODE");
                        return;
                    }
                    const std::string &targetNick = params[nextArgId++];
                    Client *targetClient = NULL;
                    std::vector<Client *> members = channels[i].getMembers();
                    for (size_t j = 0; j < members.size(); j++)
                    {
                        if (members[j]->getNickname() == targetNick)
                        {
                            targetClient = members[j];
                            break;
                        }
                    }
                    if (!targetClient)
                    {
                        std::string reply = ":server 441 " + client->getNickname() + " " + targetNick + " " + target + " :They aren't on that channel\r\n";
                        client->sendMessage(reply);
                        return; 
                    }
                    if (sign == '+')
                    {
                        if (!channels[i].isOperator(targetClient))
                            channels[i].addOperator(targetClient);
                    }
                    else
                    {
                        channels[i].removeOperator(targetClient);
                    }
                    modeStr += sign;
                    modeStr += 'o';
                    modeArgs += ' ' + targetNick;
                    break;
                }
                case 'l':
                {
                    if (sign == '+')
                    {
                        if (nextArgId >= params.size())
                        {
                            sendReply(client, 461, "Not enough parameters", "MODE");
                            return;
                        }
                        const std::string &limitStr = params[nextArgId];
                        for (size_t k = 0; k < limitStr.size(); k++)
                        {
                            if (!isdigit(static_cast<unsigned char>(limitStr[k])))
                            {
                                sendReply(client, 460, "User limit should be a number", "MODE");
                                return;
                            }
                        }
                        long newUserLimit = strtol(limitStr.c_str(), NULL, 10);
                        if (newUserLimit <= 0 || newUserLimit >= 1024)
                        {
                            sendReply(client, 460, "User limit should be 1 >=  <= 1024", "MODE");
                            return;
                        }
                        channels[i].setUsrlimit(static_cast<int>(newUserLimit));
                        modeStr += '+';
                        modeStr += 'l';
                        modeArgs += ' ' + limitStr;
                        nextArgId++;
                    }
                    else
                    {
                        modeStr += '-';
                        modeStr += 'l';
                        channels[i].setUsrlimit(0);
                    }
                    break;
                }
                case 'k':
                {
                    if (sign == '+')
                    {
                        if (nextArgId >= params.size())
                        {
                            sendReply(client, 461, "Not enough parameters", "MODE");
                            return;
                        }
                        channels[i].setPass(params[nextArgId]);
                        modeStr += '+';
                        modeStr += 'k';
                        modeArgs += ' ' + params[nextArgId];
                        nextArgId++;
                    }
                    else
                    {
                        channels[i].setPass("");
                        modeStr += '-';
                        modeStr += 'k';
                    }
                    break;
                }
                case 't':
                {
                    if (sign == '+')
                        channels[i].setTopicRestricted(true);
                    else
                        channels[i].setTopicRestricted(false);
                    modeStr += sign;
                    modeStr += 't';
                    break;
                }
                default:
                {
                    sendReply(client, 472, std::string("Unknown mode flag ") + flag, "MODE");
                    return;
                }
            }
        }
    if (!modeStr.empty())
    {
        std::string msg = ":" + client->getNickname() + "!" + client->getUsername()
                        + "@localhost MODE " + target + " " + modeStr + modeArgs + "\r\n";
        std::vector<Client *> members = channels[i].getMembers();
        for (size_t j = 0; j < members.size(); j++)
            members[j]->sendMessage(msg);
    }
    return;
    }
}