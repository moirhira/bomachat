#include "Bot.hpp"


Bot::Bot(int port, std::string address) : _sPort(port), _sAddress(address) {}
Bot::~Bot() {}


int Bot::getServerPort() {
    return _sPort;
}

std::string  Bot::getServerAdr() {
    return _sAddress;
}


void Bot::init(std::string nickName, std::string userName, std::string realName) {
    setNickname(nickName);
    setUsername(userName);
    setRealname(realName);

    int _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        perror("socket fail: ");
        return;
    }
    setFd(_sockfd);
    connectToServer();
};

void Bot::connectToServer() {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(_sPort);
    if (connect(getFd(), (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
    {
        perror("connect failed: ");
        return;
    }
    
}


