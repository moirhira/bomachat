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


void Server::parseCommand(std::string cmdLine)
{

}

void Server::handelClient(int& i) {
    char buffer[1024];
    int byts = recv(_fds[i].fd, buffer, sizeof(buffer) - 1, 0);
    if (byts <= 0)
    {
        close(_fds[i].fd);
        _fds[i] = _fds[_nfds - 1];
        _nfds--;
        i--;
    }
    else
    {
        buffer[byts] = '\0';
        Client* curClient = getClientById(_fds[i].fd);
        buffer[byts] = '\0';
        curClient->appendToBuffer(buffer);
        std::cout << "client sent this : " << buffer << std::endl;
        if (curClient->getBuffer().find("\r\n") != std::string::npos)
        {
            parseCommand(curClient->getBuffer());
            curClient->clearBuffer();
        }
    }
    // printf("buffer now -> %s\n", getClientById(_fds[i].fd)->getBuffer().c_str());
}