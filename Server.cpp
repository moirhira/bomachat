#include "Server.hpp"

Server::Server(int port, std::string password) : _port(port), _password(password) {}
Server::~Server() {}



int Server::init() {
    _sockfd = socket(AF_INET, SOCK_STREAM, 0);
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
    run();
    return 0;
}

void Server::run() { 

    _fds[0].fd = _sockfd;
    _fds[0].events = POLLIN;
    int _nfds = 1;

    while (1)
    {
        poll(_fds, _nfds, -1);
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
void Server::acceptClient() {
    int client_fd = accept(_sockfd, NULL, NULL);
    _fds[_nfds].fd = client_fd;
    _fds[_nfds].events = POLLIN;
    _nfds++;
}
void Server::handelClient(int i) {
{
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
        std::cout << "client snet this : " << buffer << std::endl;
    }
}