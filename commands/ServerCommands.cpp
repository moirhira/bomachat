#include "ServerCommands.hpp"
#include "Server.hpp"

void sendReply(Client *client, std::string Code, std::string Msg, std::string cmd)
{
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    std::string commandPart = cmd.empty() ? "" : " " + cmd;
    client->sendMessage(":server " + Code + " " + nick + commandPart + " :" + Msg + "\r\n");
}

void sendWelcome(Client *client)
{
    sendReply(client, "001", "Welcome to the IRC server " + client->getNickname(), "");
    sendReply(client, "002", "Your host is ircserv", "");
    sendReply(client, "003", "This server was created today", "");
    sendReply(client, "004", "ircserv", "");
}

bool isPreRegistrationCommand(const std::string &cmd)
{
    return (cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "CAP" || cmd == "PING" || cmd == "PONG" || cmd == "QUIT");
}

std::vector<std::string> splitCommaList(const std::string& list)
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

void Server::handelCommand(command cmd, Client *client, int& i)
{
    if (cmd.command.empty())
        return;

    if (!client->isReg() && !isPreRegistrationCommand(cmd.command))
    {
        sendReply(client, "451", "You have not registered", cmd.command);
        return;
    }

    if (cmd.command == "CAP")
    {
        if (cmd.params.empty())
            return;
        std::string sub = cmd.params[0];
        if (sub == "LS")
        {
            std::string reply = ":server CAP * LS :\r\n";
            client->sendMessage(reply);
        }
        else if (sub == "REQ" && cmd.params.size() > 1)
        {
            std::string reply = ":server CAP * NAK :" + cmd.params[1] + "\r\n";
            client->sendMessage(reply);
        }
    }
    else if (cmd.command == "PING")
    {
        std::string token;
        if (!cmd.params.empty())
            token = cmd.params[0];
        std::string reply = ":server PONG server :" + token + "\r\n";
        client->sendMessage(reply);
    }
    else if (cmd.command == "PONG")
        return;
    else if (cmd.command == "PASS")
        handlePass(client, cmd.params, _password);
    else if (cmd.command == "NICK")
        handleNick(client, cmd.params, _clients, _channels);
    else if (cmd.command == "USER")
        handleUser(client, cmd.params);
    else if (cmd.command == "JOIN")
        handleJoin(client, cmd.params, _channels);
    else if (cmd.command == "PRIVMSG")
        handlePrivmsg(client, cmd.params, _clients, _channels);
    else if (cmd.command == "TOPIC")
        handleTopic(client, cmd.params, _channels);
    else if (cmd.command == "MODE")
        handleMode(client, cmd.params, _channels);
    else if (cmd.command == "KICK")
        handleKick(client, cmd.params, _channels);
    else if (cmd.command == "INVITE")
        handleInvite(client, cmd.params, _channels, _clients);
    else if (cmd.command == "PART")
        handlePart(client, cmd.params,  _channels);
    else if (cmd.command == "QUIT")
        handleQuit(client, cmd.params, _channels, i, this);
    else
    {
        sendReply(client, "421", "Unknown command", cmd.command);
    }
}




























// void handlePass(Client *client, std::vector<std::string> params, std::string password)
// {
//     if (params.size() < 1)
//     {
//         sendReply(client, 461, "Not enough parameters", "PASS");
//         return;
//     }
//     if (client->isAuth())
//     {
//         sendReply(client, 462, "You are already registred!", "PASS");
//         return;
//     }
//     if (password != params[0])
//     {
//         sendReply(client, 464, "Worong password!", "PASS");
//         close(client->getFd());
//         return;
//     }
//     client->setAuthenticated(true);
// }

// void handleNick(Client *client, std::vector<std::string> &params, std::vector<Client *> &clients, std::vector<Channel> &channels)
// {
//     if (params.size() < 1)
//     {
//         sendReply(client, 431, "No nickname given", "NICK");
//         return;
//     }
//     if (!client->isAuth())
//     {
//         sendReply(client, 451, "You have not registered", "NICK");
//         return;
//     }
//     std::string newNick = params[0];
//     if (newNick.empty())
//     {
//         sendReply(client, 431, "No nickname given", "NICK");
//         return;
//     }

//     if (isdigit(newNick[0]) || newNick[0] == '-')
//     {
//         sendReply(client, 432, "Nickname cannot start with \"-\" or Number", "NICK");
//         return;
//     }
//     for (size_t i = 0; i < newNick.size(); i++)
//     {
//         if (!isalnum(newNick[i]) && newNick[i] != '-' && newNick[i] != '_' && newNick[i] != '[' && newNick[i] != ']' && newNick[i] != '\\' && newNick[i] != '^' && newNick[i] != '{' && newNick[i] != '}' && newNick[i] != '|')
//         {
//             sendReply(client, 432, "Nickname can only contain letters, digits, and - _ ' [ ] \\ ^ { } |", "NICK");
//             return;
//         }
//     }
//     if (newNick.size() > 9)
//     {
//         sendReply(client, 432, "Nickname too long", "NICK");
//         return;
//     }
//     for (size_t j = 0; j < clients.size(); j++)
//     {
//         if (clients[j]->getFd() != client->getFd() && clients[j]->getNickname() == newNick)
//         {
//             sendReply(client, 433, "Nickname is already in use", newNick);
//             return;
//         }
//     }
//     bool wasReg = client->isReg();
//     std::string oldNick = client->getNickname();

//     client->setNickname(newNick);

//     if (!client->getUsername().empty())
//         client->setRegistered(true);

//     if (client->isReg() && !wasReg)
//     {
//         sendWelcome(client);
//     }
//     else if (wasReg)
//     {
//         std::string nickMsg = ":" + oldNick + "!" + client->getUsername() + "@localhost NICK :" + newNick + "\r\n";
//         for (size_t j = 0; j < channels.size(); j++)
//         {
//             channels[j].brodcastMessage(nickMsg, client);
//         }
//         client->sendMessage(nickMsg);
//     }
// }

// void handleUser(Client *client, const std::vector<std::string> params)
// {
//     if (!client->isAuth())
//     {
//         sendReply(client, 451, "You have not registered", "USER");
//         return;
//     }
//     if (client->isReg())
//     {
//         sendReply(client, 462, "You are already registred!", "USER");
//         return;
//     }
//     if (params.size() < 4)
//     {
//         sendReply(client, 461, "Not enough parameters", "USER");
//         return;
//     }
//     client->setUsername(params[0]);
//     client->setRealname(params[3]);
//     if (!client->getNickname().empty())
//     {
//         client->setRegistered(true);
//     }
//     if (client->isReg())
//         sendWelcome(client);
// }

// static void sendJoinReply(Client *client, Channel &channel)
// {
//     std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost JOIN " + channel.getName() + "\r\n";
//     client->sendMessage(joinMsg);

//     std::string nick = client->getNickname();
//     std::string channnelName = channel.getName();

//     if (!channel.getTopic().empty())
//     {
//         std::string topicReply = ":server 332 " + nick + " " + channnelName + " :" + channel.getTopic() + "\r\n";
//         client->sendMessage(topicReply);
//     }
//     else
//     {
//         std::string noTopic = ":server 331 " + nick + " " + channnelName + " :NO topic is set\r\n";
//         client->sendMessage(noTopic);
//     }

//     std::string namesList = ":server 353 " + nick + " = " + channnelName + " :";
//     std::vector<Client *> members = channel.getMembers();
//     for (size_t i = 0; i < members.size(); i++)
//     {
//         if (channel.isOperator(members[i]))
//             namesList += "@";
//         namesList += members[i]->getNickname();
//         if (i + 1 < members.size())
//             namesList += " ";
//     }

//     namesList += "\r\n";
//     client->sendMessage(namesList);

//     std::string endNames = ":server 366 " + nick + " " + channnelName + " :End of /NAMES list\r\n";
//     client->sendMessage(endNames);
// }

// bool isValidChannelName(const std::string& name)
// {
//     if (name.empty() || name[0] != '#' || name.size() == 1)
//         return false;
//     for (size_t i = 1;  i < name.size(); i++)
//     {
//         char c = name[i];

//         if (c == ' ' || c == ',' || c < 32 || c == 127)
//             return false;
//     }
//     return true;
// }



// void handleJoin(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
// {
//     if (params.size() < 1)
//     {
//         sendReply(client, 461, "Not enough parameters", "JOIN");
//         return;
//     }
//     if (!client->isReg())
//     {
//         sendReply(client, 451, "You have not registered", "JOIN");
//         return;
//     }
//     std::vector<std::string> channelList = splitCommaList(params[0]);
//     std::vector<std::string> keyList;
//     if (params.size() > 1)
//         keyList = splitCommaList(params[1]);
    
//     for (size_t i = 0; i < channelList.size(); i++)
//     {
//         std::string channelName = channelList[i];
//         std::string key;
//         if (i < keyList.size())
//             key = keyList[i];
//         else
//             key = "";

//         if (!isValidChannelName(channelName))
//         {
//             sendReply(client, 476, "Channel name is invalid", channelName);
//             continue;
//         }
//         bool found = false;
//         for (size_t j = 0; j < channels.size(); j++)
//         {
//             if (channelName == channels[j].getName())
//             {
//                 found = true;
//                 if (channels[j].isMember(client))
//                     break;
//                 if (channels[j].isInviteOnly() && !channels[j].isInvited(client))
//                 {
//                     sendReply(client, 473, "You are not invited to this channel", channelName);
//                     break;
//                 }
//                 if (!channels[j].getPass().empty())
//                 {
//                     if (channels[j].getPass() != key)
//                     {
//                         sendReply(client, 475, "Invalid channel password", channelName);
//                         break;
//                     }
//                 }
//                 if (channels[j].getUserlimit() > 0 && channels[j].getUserlimit() <= static_cast<int>(channels[j].getMembers().size()))
//                 {
//                     sendReply(client, 471, "Channel is full", channelName);
//                     break;
//                 }
//                 channels[j].addMember(client);
//                 sendJoinReply(client, channels[j]);
//                 break;
//             }
//         }
//         if (!found)
//         {
//             channels.push_back(Channel(channelName, client));
//             sendJoinReply(client, channels.back());
//         }
//     }
// }

// void handlePrivmsg(Client *client, std::vector<std::string> params, std::vector<Client *> &clients, std::vector<Channel> &channels)
// {
//     if (params.size() == 0)
//     {
//         sendReply(client, 411, "No recipient given (PRIVMSG)", "PRIVMSG");
//         return;
//     }
//     if (params.size() == 1)
//     {
//         sendReply(client, 412, "No text to send", "PRIVMSG");
//         return;
//     }
//     std::string target = params[0];
//     std::string msg = params[1];
//     if (target[0] == '#')
//     {
//         for (size_t i = 0; i < channels.size(); i++)
//         {
//             if (channels[i].getName() == target)
//             {
//                 if (!channels[i].isMember(client))
//                 {
//                     sendReply(client, 442, "You are not on that channel", target);
//                     return;
//                 }

//                 std::vector<Client *> members = channels[i].getMembers();
//                 for (size_t j = 0; j < channels[i].getMembers().size(); j++)
//                 {
//                     if (client->getFd() != members[j]->getFd())
//                     {
//                         std::string prvMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PRIVMSG " + target + " :" + msg + "\r\n";
//                         members[j]->sendMessage(prvMsg);
//                     }
//                 }
//                 return;
//             }
//         }
//         sendReply(client, 403, "Channel doesn't exist", target);
//         return;
//     }

//     for (size_t i = 0; i < clients.size(); i++)
//     {
//         if (clients[i]->getNickname() == target)
//         {
//             std::string prvMsg = ":" + client->getNickname() + "!" +
//                                  client->getUsername() + "@localhost PRIVMSG " +
//                                  target + " :" + msg + "\r\n";
//             clients[i]->sendMessage(prvMsg);
//             return;
//         }
//     }
//     sendReply(client, 401, "No such nick/channel", target);
// }

// void handleTopic(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
// {
//     if (params.size() < 1)
//     {
//         sendReply(client, 461, "Not enough parameters", "TOPIC");
//         return;
//     }
//     if (params[0][0] != '#')
//     {
//         sendReply(client, 476, "Channel name should start with #", params[0]);
//         return;
//     }
//     for (size_t i = 0; i < channels.size(); i++)
//     {
//         if (channels[i].getName() == params[0])
//         {
//             if (!channels[i].isMember(client))
//             {
//                 sendReply(client, 442, "You are not on that channel", params[0]);
//                 return;
//             }
//             if (params.size() == 1)
//             {
//                 if (!channels[i].getTopic().empty())
//                     sendReply(client, 332, channels[i].getTopic(), params[0]);
//                 else
//                     sendReply(client, 331, "No topic is set", params[0]);
//                 return;
//             }
//             if (channels[i].isTopicRestricted() && !channels[i].isOperator(client))
//             {
//                 sendReply(client, 482, "You are not allowed to change the topic", params[0]);
//                 return;
//             }
//             channels[i].setTopic(params[1]);
//             std::string topicMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost TOPIC " + params[0] + " :" + params[1] + "\r\n";
//             std::vector<Client *> members = channels[i].getMembers();
//             for (size_t j = 0; j < channels[i].getMembers().size(); j++)
//             {
//                 members[j]->sendMessage(topicMsg);
//             }
//             return;
//         }
//     }
//     sendReply(client, 403, "Channel doesn't exist", params[0]);
// }

// void handleMode(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
// {
//     if (params.size() < 1)
//     {
//         sendReply(client, 461, "Not enough parameters", "MODE");
//         return;
//     }
//     std::string target = params[0];
//     if (target[0] != '#')
//     {
//         if (target != client->getNickname())
//         {
//             sendReply(client, 502, "Can't change mode for other users", "MODE");
//             return;
//         }
//         std::string reply = ":server 221 " + client->getNickname() + " +\r\n";
//         client->sendMessage(reply);
//         return;
//     }

//     if (params.size() == 1)
//     {
//         for (size_t i = 0; i < channels.size(); i++)
//         {
//             if (channels[i].getName() != target)
//                 continue;
            
//             std::string modeStr = "+";
//             std::string modeArgs;
//             if (channels[i].isInviteOnly())
//                 modeStr += 'i';
//             if (channels[i].isTopicRestricted())
//                 modeStr += 't';
//             if (!channels[i].getPass().empty())
//             {
//                 modeStr += 'k';
//                 modeArgs += " " + channels[i].getPass();
//             }
//             if (channels[i].getUserlimit() > 0)
//             {
//                 modeStr += 'l';
//                 std::ostringstream oss;
//                 oss << channels[i].getUserlimit();
//                 modeArgs += " " + oss.str();
//             }

//             std::string reply = ":server 324 " + client->getNickname() + " " + target + " " + modeStr + modeArgs + "\r\n";
//             client->sendMessage(reply);
//             return;
//         }
//         sendReply(client, 403, "Channel doesn't exist", "MODE");
//         return;
//     }
    

//     if (!client->isReg())
//     {
//         sendReply(client, 451, "You have not registered", "MODE");
//         return;
//     }

//     if (params[1].empty() || (params[1][0] != '+' && params[1][0] != '-'))
//     {
//         sendReply(client, 472, "Unknown mode flag", "MODE");
//         return;
//     }

//     for (size_t i = 0; i < channels.size(); i++)
//     {
//         if (channels[i].getName() != target)
//             continue;

//         if (!channels[i].isOperator(client))
//         {
//             sendReply(client, 482, "You're not channel operator", "MODE");
//             return;
//         }

//         char sign = '+';
//         size_t nextArgId = 2;
//         std::string modeStr;
//         std::string modeArgs;

//         for  (size_t f = 0; f < params[1].size(); f++)
//         {
//             char flag = params[1][f];
//             if (flag == '+' || flag == '-')
//             {
//                 sign = flag;
//                 continue;
//             }

//             switch (flag)
//             {
//                 case 'i':
//                 {
//                     if (sign == '+')
//                         channels[i].setInviteOnly(true);
//                     else
//                         channels[i].setInviteOnly(false);
//                     modeStr += sign;
//                     modeStr += 'i';
//                     break;
//                 }
//                 case 'o':
//                 {
//                     if (nextArgId >= params.size())
//                     {
//                         sendReply(client, 461, "Not enough parameters", "MODE");
//                         return;
//                     }
//                     const std::string &targetNick = params[nextArgId++];
//                     Client *targetClient = NULL;
//                     std::vector<Client *> members = channels[i].getMembers();
//                     for (size_t j = 0; j < members.size(); j++)
//                     {
//                         if (members[j]->getNickname() == targetNick)
//                         {
//                             targetClient = members[j];
//                             break;
//                         }
//                     }
//                     if (!targetClient)
//                     {
//                         std::string reply = ":server 441 " + client->getNickname() + " " + targetNick + " " + target + " :They aren't on that channel\r\n";
//                         client->sendMessage(reply);
//                         return; 
//                     }
//                     if (sign == '+')
//                     {
//                         if (!channels[i].isOperator(targetClient))
//                             channels[i].addOperator(targetClient);
//                     }
//                     else
//                     {
//                         channels[i].removeOperator(targetClient);
//                     }
//                     modeStr += sign;
//                     modeStr += 'o';
//                     modeArgs += ' ' + targetNick;
//                     break;
//                 }
//                 case 'l':
//                 {
//                     if (sign == '+')
//                     {
//                         if (nextArgId >= params.size())
//                         {
//                             sendReply(client, 461, "Not enough parameters", "MODE");
//                             return;
//                         }
//                         const std::string &limitStr = params[nextArgId];
//                         for (size_t k = 0; k < limitStr.size(); k++)
//                         {
//                             if (!isdigit(static_cast<unsigned char>(limitStr[k])))
//                             {
//                                 sendReply(client, 460, "User limit should be a number", "MODE");
//                                 return;
//                             }
//                         }
//                         long newUserLimit = strtol(limitStr.c_str(), NULL, 10);
//                         if (newUserLimit <= 0 || newUserLimit >= 1024)
//                         {
//                             sendReply(client, 460, "User limit should be 1 >=  <= 1024", "MODE");
//                             return;
//                         }
//                         channels[i].setUsrlimit(static_cast<int>(newUserLimit));
//                         modeStr += '+';
//                         modeStr += 'l';
//                         modeArgs += ' ' + limitStr;
//                         nextArgId++;
//                     }
//                     else
//                     {
//                         modeStr += '-';
//                         modeStr += 'l';
//                         channels[i].setUsrlimit(0);
//                     }
//                     break;
//                 }
//                 case 'k':
//                 {
//                     if (sign == '+')
//                     {
//                         if (nextArgId >= params.size())
//                         {
//                             sendReply(client, 461, "Not enough parameters", "MODE");
//                             return;
//                         }
//                         channels[i].setPass(params[nextArgId]);
//                         modeStr += '+';
//                         modeStr += 'k';
//                         modeArgs += ' ' + params[nextArgId];
//                         nextArgId++;
//                     }
//                     else
//                     {
//                         channels[i].setPass("");
//                         modeStr += '-';
//                         modeStr += 'k';
//                     }
//                     break;
//                 }
//                 case 't':
//                 {
//                     if (sign == '+')
//                         channels[i].setTopicRestricted(true);
//                     else
//                         channels[i].setTopicRestricted(false);
//                     modeStr += sign;
//                     modeStr += 't';
//                     break;
//                 }
//                 default:
//                 {
//                     sendReply(client, 472, std::string("Unknown mode flag ") + flag, "MODE");
//                     return;
//                 }
//             }
//         }
//     if (!modeStr.empty())
//     {
//         std::string msg = ":" + client->getNickname() + "!" + client->getUsername()
//                         + "@localhost MODE " + target + " " + modeStr + modeArgs + "\r\n";
//         std::vector<Client *> members = channels[i].getMembers();
//         for (size_t j = 0; j < members.size(); j++)
//             members[j]->sendMessage(msg);
//     }
//     return;
//     }
// }

// void handleKick(Client *client, std::vector<std::string> params, std::vector<Channel> &channels)
// {
//     if (params.size() < 2)
//     {
//         sendReply(client, 461, "Not enough parameters", "KICK");
//         return;
//     }
//     if (params[0][0] != '#')
//     {
//         sendReply(client, 476, "Channel name should start with #", params[0]);
//         return;
//     }
//     std::string targetChannel = params[0];
//     std::string targetUser = params[1];
//     for (size_t i = 0; i < channels.size(); i++)
//     {
//         if (channels[i].getName() == targetChannel)
//         {
//             Channel &channel = channels[i];
//             if (channel.isMember(client))
//             {
//                 if (channel.isOperator(client))
//                 {
//                     std::vector<Client *> members = channel.getMembers();
//                     for (size_t m = 0; m < members.size(); m++)
//                     {
//                         if (members[m]->getNickname() == targetUser)
//                         {
//                             std::string msgReply;
//                             if (params.size() == 2)
//                                 msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost KICK " + params[0] + " " + params[1] + "\r\n";
//                             else
//                                 msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost KICK " + params[0] + " " + params[1] + " :" + params[2] + "\r\n";

//                             for (size_t j = 0; j < members.size(); j++)
//                             {
//                                 members[j]->sendMessage(msgReply);
//                             }
//                             channel.removeOperator(members[m]);
//                             channel.removeMember(members[m]);
//                             channel.removeFromInviteList(members[m]);
//                             return;
//                         }
//                     }
//                     sendReply(client, 441, "They aren't on that channel", targetUser + " " + targetChannel);
//                     return;
//                 }
//                 else
//                 {
//                     sendReply(client, 482, "You're not channel operator", targetChannel);
//                     return;
//                 }
//             }
//             else
//             {
//                 sendReply(client, 442, "You are not on that channel", targetChannel);
//                 return;
//             }
//         }
//     }
//     sendReply(client, 403, "Channel doesn't exist", targetChannel);
//     return;
// }

// void handleInvite(Client *client, std::vector<std::string> params, std::vector<Channel> &channels, std::vector<Client *> &clients)
// {
//     if (params.size() < 2)
//     {
//         sendReply(client, 461, "Not enough parameters", "INVITE");
//         return;
//     }
//     std::string targetClientNick = params[0];
//     std::string targetChannel = params[1];

//     Client *target = NULL;
//     for (size_t j = 0; j < clients.size(); j++)
//     {
//         if (clients[j]->getNickname() == targetClientNick)
//         {
//             target = clients[j];
//             break;
//         }
//     }
//     if (!target)
//     {
//         sendReply(client, 401, "No such nick", targetClientNick);
//         return;
//     }

//     for (size_t i = 0; i < channels.size(); i++)
//     {
//         if (channels[i].getName() == targetChannel)
//         {
//             if (!channels[i].isMember(client))
//             {
//                 sendReply(client, 442, "You are not on that channel", targetChannel);
//                 return;
//             }

//             if (!channels[i].isOperator(client))
//             {
//                 sendReply(client, 482, "You're not channel operator", targetChannel);
//                 return;
//             }

//             if (channels[i].isMember(target))
//             {
//                 sendReply(client, 443, "is already on channel", targetClientNick + " " + targetChannel);
//                 return;
//             }

//             channels[i].addToInviteList(target);

//             std::string senderMsg = ":server 341 " + client->getNickname() + " " + targetClientNick + " " + targetChannel + "\r\n";
//             client->sendMessage(senderMsg);
//             std::string msgReply = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost INVITE " + targetClientNick + " " + targetChannel + "\r\n";
//             target->sendMessage(msgReply);

//             return;
//         }
//     }
//     sendReply(client, 403, "No such channel", targetChannel);
// }


// void handlePart(Client *client, std::vector<std::string> params, std::vector<Channel> &channels) {
//     (void)channels;
//     if (params.size() < 1)
//     {
//         sendReply(client, 461, "Not enough parameters", "PART");
//         return;
//     }
//     std::string reason;
//     if (params.size() > 1)
//         reason = params[1];
//     std::vector<std::string> channelList = splitCommaList(params[0]);
//     for (size_t i = 0; i < channelList.size(); i++)
//     {
//         std::string channelName = channelList[i];
//         if (channelName[0] != '#')
//         {
//             sendReply(client, 476, "Channel name should start with #", channelName);
//             return;
//         }
//         bool found = false;
//         for (size_t j = 0; j < channels.size(); j++)
//         {
//             if (channelName == channels[j].getName())
//             {
//                 found = true;
//                 if (!channels[j].isMember(client))
//                 {
//                     sendReply(client, 476, "You are not on that channel",channelName);
//                     break;
//                 }
//                 channels[j].removeClientEverywhere(client);
//                 std::string partMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost PART " + channelName;
//                 if (!reason.empty())
//                     partMsg += " :" + reason;
//                 partMsg += "\r\n";
//                 channels[j].brodcastMessage(partMsg, client);
//                 client->sendMessage(partMsg);
//                 break;
//             }
//         }
//         if (!found)
//         {
//             sendReply(client, 403, "Channel doesn't exist", channelName);
//             return;
//         }
//     }
// }


// void handleQuit(Client *client, std::vector<std::string> params, std::vector<Channel> &channels, int& i, Server *server) {
//     std::string reason;
//     if (params.size() > 0)
//         reason = params[0];
//     else
//         reason = "Client Quit";
//     std::string quirtMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :" + reason + "\r\n";
//     for (size_t j = 0; j < channels.size(); j++)
//     {
//         if (!channels[j].isMember(client))
//             continue;
//         channels[j].brodcastMessage(quirtMsg, client);
//         channels[j].removeClientEverywhere(client);
//     }
//     server->disconnectClient(i);
// }