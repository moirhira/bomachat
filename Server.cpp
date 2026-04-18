#include "Server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password) {}
Server::~Server()
{
    for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
}

static bool isPreRegistrationCommand(const std::string &cmd)
{
    return (cmd == "KICK" || cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "CAP" || cmd == "PING" || cmd == "PONG");
}

void Server::disconnectClient(int &i)
{
    int fd = _fds[i].fd;
    close(fd);
    _fds[i] = _fds[_nfds - 1];
    _nfds--;
    i--;
    Client* client = getClientById(fd);
    if (client)
    {
        std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :connection closed\r\n";

        for(size_t c = 0; c < _channels.size(); c++)
        {
            if (_channels[c].isMember(client))
            {
                std::vector<Client*> members = _channels[c].getMembers();
                for (size_t j = 0; j < members.size(); j++)
                {
                    if (members[j]->getFd() != fd)
                    {
                        members[j]->sendMessage(quitMsg);
                    }
                }

            }
        }
        for (size_t c = 0; c < _channels.size(); c++)
        {
            _channels[c].removeClientEverywhere(client);
        }
    }

    for (size_t c = 0; c < _channels.size(); c++)
    {
        if (_channels[c].isEmpty())
        {
            _channels.erase(_channels.begin() + c);
            c--;
        }
    }

    for (size_t j = 0; j < _clients.size(); j++)
    {
        if (_clients[j]->getFd() == fd)
        {
            delete _clients[j];
            _clients.erase(_clients.begin() + j);
            break;
        }
    }
}

int Server::init()
{
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (_sockfd < 0)
    {
        perror("socket fail: ");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(_port);
    if (bind(_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("bind faild: ");
        return 1;
    }

    if (listen(_sockfd, 10) < 0)
    {
        perror("listen faild: ");
        return 1;
    }
    if (fcntl(_sockfd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl faild: ");
        return 1;
    }
    return 0;
}

void Server::run()
{
    memset(_fds, 0, sizeof(_fds));
    _fds[0].fd = _sockfd;
    _fds[0].events = POLLIN;
    _nfds = 1;

    while (1)
    {
        for (int i = 1; i < _nfds; i++)
        {
            Client *client = getClientById(_fds[i].fd);
            if (client)
                _fds[i].events = client->getClientEvents();
            else
                _fds[i].events = POLLIN;
        }

        if (poll(_fds, _nfds, -1) < 0)
        {
            perror("poll faild: ");
            return;
        }
        for (int i = 0; i < _nfds; i++)
        {
            if (_fds[i].revents & (POLLERR | POLLHUP | POLLNVAL))
            {
                disconnectClient(i);
                continue;
            }
            if (_fds[i].revents & POLLOUT)
            {
                Client* client = getClientById(_fds[i].fd);
                if (!client)
                    continue;

                std::string &out = client->getOutBuffer();
                if (out.empty())
                    continue;

                ssize_t sent = send(client->getFd(), out.c_str(), out.size(), 0);

                if (sent > 0)
                {
                    out.erase(0, static_cast<size_t>(sent));
                }
                else if (sent < 0)
                {
                    if (errno == EWOULDBLOCK || errno == EAGAIN)
                        continue;
                    disconnectClient(i);
                    continue;
                }
                else
                {
                    disconnectClient(i);
                    continue;
                }
            }
            if (_fds[i].revents & POLLIN)
            {
                if (_fds[i].fd == _sockfd)
                {
                    acceptClient();
                }
                else
                {
                    handelClient(i);
                }
            }
        }
    }
}

Client *Server::getClientById(int id)
{
    for (size_t i = 0; i < _clients.size(); i++)
    {
        if (_clients[i]->getFd() == id)
            return _clients[i];
    }
    return NULL;
}

void Server::acceptClient()
{
    if (_nfds >= 1024)
    {
        std::cerr << "Max clients reached, skipping accept" << std::endl;
        return;
    }
    int client_fd = accept(_sockfd, NULL, NULL);
    if (client_fd < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return;
        
        if (errno == EMFILE)
        {
            std::cerr << "FD limit reached, refusing connections" << std::endl;
            return;
        }
        perror("accept faild: ");
        return;
    }
    if (client_fd >= 1024)
    {
        std::cerr << "FD limit reached, refusing connections" << std::endl;
        close(client_fd);
        return;
    }
    if (fcntl(client_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl faild: ");
        close(client_fd);
        return;
    }
    _clients.push_back(new Client(client_fd));
    _fds[_nfds].fd = client_fd;
    _fds[_nfds].events = _clients.back()->getClientEvents();
    _nfds++;
    std::cout << "new client connected" << std::endl;
}

command Server::parseCommand(std::string cmdLine)
{
    command cmdStruct;

    if (cmdLine.empty())
        return cmdStruct;

    size_t start = cmdLine.find_first_not_of(" \t\n\r");
    if (start == std::string::npos)
        return cmdStruct;
    std::string trimmedCmd = cmdLine.substr(start);

    if (trimmedCmd[0] == ':')
    {
        size_t spacePos = trimmedCmd.find(' ');
        if (spacePos == std::string::npos)
            return cmdStruct;
        trimmedCmd = trimmedCmd.substr(spacePos + 1);
    }

    size_t spacePos = trimmedCmd.find(' ');
    cmdStruct.command = trimmedCmd.substr(0, spacePos);

    for (size_t i = 0; i < cmdStruct.command.size(); i++)
    {
        cmdStruct.command[i] = toupper(cmdStruct.command[i]);
    }

    if (spacePos == std::string::npos)
        return cmdStruct;
    trimmedCmd.erase(0, spacePos + 1);

    while (!trimmedCmd.empty())
    {
        if (trimmedCmd[0] == ':')
        {
            cmdStruct.params.push_back(trimmedCmd.substr(1));
            break;
        }
        size_t pos = trimmedCmd.find(' ');
        if (pos == std::string::npos)
        {
            cmdStruct.params.push_back(trimmedCmd);
            break;
        }
        cmdStruct.params.push_back(trimmedCmd.substr(0, pos));
        trimmedCmd.erase(0, pos + 1);
    }
    return cmdStruct;
}

void sendReply(Client *client, int errorCode, std::string errorMsg, std::string cmd)
{
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    std::ostringstream oss;
    oss << std::setw(3) << std::setfill('0') << errorCode;
    std::string reply = ":server " + oss.str() + " " + nick + " " + cmd + " :" + errorMsg + "\r\n";
    client->sendMessage(reply);
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

void sendWelcome(Client *client)
{
    sendReply(client, 001, "Welcome to the IRC server " + client->getNickname(), "");
    sendReply(client, 002, "Your host is ircserv", "");
    sendReply(client, 003, "This server was created today", "");
    sendReply(client, 004, "ircserv", "");
}

void handleNick(Client *client, std::vector<std::string> &params, std::vector<Client *> &clients)
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
    for (size_t j = 0; j < clients.size(); j++)
    {
        if (clients[j]->getFd() != client->getFd() && clients[j]->getNickname() == newNick)
        {
            sendReply(client, 433, "nickname already taken by another client", "NICK");
            return;
        }
    }
    client->setNickname(newNick);
    if (!client->getUsername().empty())
        client->setRegistered(true);
    if (client->isReg())
        sendWelcome(client);
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
    std::string joinMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@host JOIN " + channel.getName() + "\r\n";
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
    if (params[0][0] != '#')
    {
        sendReply(client, 476, "Channel name should start with #", "JOIN");
        return;
    }
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (params[0] == channels[i].getName())
        {
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
            if (channels[i].isMember(client))
                return;
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
                        std::string prvMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@<host> PRIVMSG " + target + " :" + msg + "\r\n";
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
        sendReply(client, 502, "Can't change mode for other users", "MODE");
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
                    channels[i].addOperator(targetClient);
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
                    if (newUserLimit < 0 || newUserLimit >= 1024)
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
            if (channels[i].isMember(client))
            {
                if (channels[i].isOperator(client))
                {
                    std::vector<Client *> members = channels[i].getMembers();
                    for (size_t i = 0; i < members.size(); i++)
                    {
                        if (members[i]->getNickname() == targetUser)
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
                            channels[i].removeOperator(members[i]);
                            channels[i].removeMember(members[i]);
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

void Server::handelCommand(command cmd, Client *client)
{

    if (cmd.command.empty())
        return;

    if (!client->isReg() && !isPreRegistrationCommand(cmd.command))
    {
        sendReply(client, 451, "You have not registered", cmd.command);
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
        handleNick(client, cmd.params, _clients);
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
}
void Server::handelClient(int &i)
{
    char buffer[1024];
    int byts = recv(_fds[i].fd, buffer, sizeof(buffer) - 1, 0);
    if (byts < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return;
        perror("recv failed: ");
        return;
    }
    if (byts <= 0)
    {
        disconnectClient(i);
        std::cout << "client disconnected" << std::endl;
    }
    else
    {
        buffer[byts] = '\0';
        Client *curClient = getClientById(_fds[i].fd);
        if (!curClient)
            return;
        curClient->appendToBuffer(std::string(buffer, byts));
        std::string &cmdLine = curClient->getBuffer();
        size_t pos;
        while ((pos = cmdLine.find("\r\n")) != std::string::npos)
        {
            std::string line = cmdLine.substr(0, pos);
            cmdLine.erase(0, pos + 2);
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            command cmd = parseCommand(line);
            handelCommand(cmd, curClient);
        }
    }
    // printf("buffer now -> %s\n", getClientById(_fds[i].fd)->getBuffer().c_str());
}
