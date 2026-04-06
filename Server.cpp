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

    struct pollfd fds[1024];
    fds[0].fd = _sockfd;
    fds[0].events = POLLIN;
    int nfds = 1;

    while (1)
    {
        poll(fds, nfds, -1);

        for (int i = 0; i < nfds, i++)
        {
            if (fds[i].revents & POLLIN)
            {
                if (fds[i].fd == _sockfd)
                {
                    acceptClient();
                }
                else
                {
                    handelClient();
                }
            }
        }
    }

}
void Server::acceptClient() {
    int client_fd = accept(sockfd, NULL, NULL);
    fds[nfds].fd = client_fd;
    fds[nfds].events = POLLIN;
    nfds++;
}
void Server::handelClient() {
{
    char buffer[1024];
    int byts = recv(fds[nfds].fd, buffer, sizeof(buffer) - 1, 0);
    if (byts <= 0)
    {
        close(fds[nfds]);
        fds[i] = fds[nfds - 1];
        nfds--;
        i--;
    }
    else
    {
        

    }
}