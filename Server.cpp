#include "Server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password) {}
Server::~Server() {}



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


void handlePass(Client* client, std::vector<std::string> params) {

}

void handleNick(Client* client, std::vector<std::string> params) {
    
}

void handleUser(Client* client, std::vector<std::string> params) {
    
}
void handleJoin(Client* client, std::vector<std::string> params) {
    
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
    if (cmd.command == "PASS")
        handlePass(client, cmd.params);
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
        while ((pos = cmdLine.find("\r\n")) != std::string::npos)
        {
            std::string line = cmdLine.substr(0, pos);
            cmdLine.erase(0, pos + 2);
            command cmd = parseCommand(line);
            handelCommand(cmd, curClient);
        }
    }
    // printf("buffer now -> %s\n", getClientById(_fds[i].fd)->getBuffer().c_str());
}