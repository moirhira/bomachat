#include "Server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password) {}
Server::~Server() {}

static bool isPreRegistrationCommand(const std::string& cmd)
{
    return (cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "CAP");
}



int Server::init() {
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
    return 0;
}

void Server::run() { 
    memset(_fds, 0, sizeof(_fds));
    _fds[0].fd = _sockfd;
    _fds[0].events = POLLIN;
    _nfds = 1;

    while (1)
    {
        if (poll(_fds, _nfds, -1) < 0)
        {
            perror("poll faild: ");
            return ;
        }
        for (int i = 0; i < _nfds; i++)
        {
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


Client* Server::getClientById(int id) {
    for (int i = 0 ; i < _clients.size(); i++)
    {
        if (_clients[i].getFd() == id)
            return &_clients[i];
    }
    return NULL;
}

void Server::acceptClient() {
    int client_fd = accept(_sockfd, NULL, NULL);
    if (client_fd < 0)
    {
        perror("accept faild: ");
        return;
    }
    if (_nfds >= 1024)
    {
        std::cerr << "Too many clients!" << std::endl;
        close(client_fd);
        return;
    }
    _clients.push_back(Client(client_fd));
    _fds[_nfds].fd = client_fd;
    _fds[_nfds].events = POLLIN;
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
            return  cmdStruct;
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

void sendReply(Client* client, int errorCode, std::string errorMsg, std::string cmd)
{
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    std::ostringstream oss;
    oss << std::setw(3) << std::setfill('0') << errorCode;
    std::string reply = ":server " + oss.str() + " " 
                            + nick + " " 
                            + cmd + " :" + errorMsg + "\r\n";
    send(client->getFd(), reply.c_str(),reply.size(), 0);
}


void handlePass(Client* client, std::vector<std::string> params, std::string password) {
    if (params.size() < 1)
    {
        sendReply(client, 461, "Not enough parameters", "PASS");
        return;
    }
    if(client->isAuth())
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

void sendWelcome(Client* client) {
    sendReply(client, 001, "Welcome to the IRC server " + client->getNickname(), "");
    sendReply(client, 002, "Your host is ircserv", "");
    sendReply(client, 003, "This server was created today", "");
    sendReply(client, 004, "ircserv", "");
}

void handleNick(Client* client, std::vector<std::string>& params, std::vector<Client>& clients) {
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
        if (!isalnum(newNick[i]) && newNick[i] != '-' && newNick[i] != '_' 
            && newNick[i] != '[' && newNick[i] != ']' && newNick[i] != '\\' 
            && newNick[i] != '^' && newNick[i] != '{' && newNick[i] != '}' && newNick[i] != '|')
        {
            sendReply(client, 432, "Nickname can only contain letters, digits, and - _ ' [ ] \\ ^ { } |", "NICK");
            return;
        }
    }
    for (size_t j = 0; j < clients.size(); j++)
    {
        if (clients[j].getFd() != client->getFd() && clients[j].getNickname() == newNick)
        {
            sendReply(client, 433, "nickname already taken by another client", "NICK");
            return;
        }
    }
    client->setNickname(newNick);
    if(!client->getUsername().empty())
         client->setRegistered(true);
    if (client->isReg())
        sendWelcome(client);
}

void handleUser(Client* client, const std::vector<std::string> params) {
    if(!client->isAuth())
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
    if(client->isReg())
        sendWelcome(client);
}


void handleJoin(Client* client, std::vector<std::string> params, std::vector<Channel>& channels) {
    if (params.size() < 2)
    {
        sendReply(client, 461, "Not enough parameters", "JOIN");
        return;
    }
     if(!client->isReg())
    {
        sendReply(client, 451, "You have not registered", "USER");
        return;
    }
    if (params[0][0] != '#')
    {
        sendReply(client, 433, "Channel name should start with #", "USER");
        return;
    }
    for (size_t i = 0; i < channels.size(); i++)
    {
        if (params[0] == channels[i].getName())
        {

        }
    }
    channels.push_back(Channel(params[0], client));
    
}


void handlePrivmsg(Client* client, std::vector<std::string> params) {
    
}

void handleKick(Client* client, std::vector<std::string> params) {
    
}

void handleInvite(Client* client, std::vector<std::string> params) {
    
}

void handleTopic(Client* client, std::vector<std::string> params) {
    
}
void handleMode(Client* client, std::vector<std::string> params) {
    
}

void Server::handelCommand(command cmd, Client* client){ 

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
            send(client->getFd(), reply.c_str(), reply.size(), 0);
        }
        else if (sub == "REQ" && cmd.params.size() > 1)
        {
            std::string reply = ":server CAP * NAK :" + cmd.params[1] + "\r\n";
            send(client->getFd(), reply.c_str(), reply.size(), 0);
        }
        // CAP END → ignore silently
    }
    else if (cmd.command == "PASS")
        handlePass(client, cmd.params, _password);
    else if (cmd.command == "NICK")
        handleNick(client, cmd.params, _clients);
    else if (cmd.command == "USER")
        handleUser(client, cmd.params);
    else if (cmd.command == "JOIN")
        handleJoin(client, cmd.params);
    else if (cmd.command == "PRIVMSG")
        handlePrivmsg(client, cmd.params);
    else if (cmd.command == "KICK")
        handleKick(client, cmd.params);
    else if (cmd.command == "INVITE")
        handleInvite(client, cmd.params);
    else if (cmd.command == "TOPIC")
        handleTopic(client, cmd.params);
    else if (cmd.command == "MODE")
        handleMode(client, cmd.params);
}
void Server::handelClient(int& i) {
    char buffer[1024];
    int byts = recv(_fds[i].fd, buffer, sizeof(buffer) - 1, 0);
    if (byts <= 0)
    {
        int fd = _fds[i].fd;
        close(fd);
        _fds[i] = _fds[_nfds - 1];
        _nfds--;
        i--;
        for (int j = 0; j < _clients.size(); j++)
        {
            if (_clients[j].getFd() == fd)            {
                _clients.erase(_clients.begin() + j);
                break;
            }
        }
        std::cout << "client disconnected" << std::endl;
    }
    else
    {
        buffer[byts] = '\0';
        Client* curClient = getClientById(_fds[i].fd);
        if (!curClient)
            return;
        curClient->appendToBuffer(buffer);
        std::string& cmdLine = curClient->getBuffer();
        size_t pos;
        while ((pos = cmdLine.find('\n')) != std::string::npos)
        {
            std::string line = cmdLine.substr(0, pos);
            cmdLine.erase(0, pos + 1);
            if (!line.empty() && line[line.size() - 1] == '\r')
                line.erase(line.size() - 1);
            command cmd = parseCommand(line);
            handelCommand(cmd, curClient);
        }
    }
    // printf("buffer now -> %s\n", getClientById(_fds[i].fd)->getBuffer().c_str());
}