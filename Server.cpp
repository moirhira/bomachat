#include "Server.hpp"
#include "ServerCommands.hpp"

void sendReply(Client *client, int errorCode, std::string errorMsg, std::string cmd);

Server::Server(int port, std::string password) : _port(port), _password(password) {}
Server::~Server()
{
    for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
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

void Server::handelCommand(command cmd, Client *client, int& i)
{
    std::cout << "Received command: " << cmd.command << std::endl;

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
        if (curClient->getBuffer().size() > 4096)
        {
            disconnectClient(i);
            std::cout << "client disconnected (buffer overflow)" << std::endl;
            return;
        }
        std::string &cmdLine = curClient->getBuffer();
        size_t pos;
        while ((pos = cmdLine.find("\r\n")) != std::string::npos)
        {
            std::string line = cmdLine.substr(0, pos);
            cmdLine.erase(0, pos + 2);
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            command cmd = parseCommand(line);
            handelCommand(cmd, curClient, i);
        }
    }
    // printf("buffer now -> %s\n", getClientById(_fds[i].fd)->getBuffer().c_str());
}
