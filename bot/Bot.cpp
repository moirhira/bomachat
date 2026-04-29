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
};

void Bot::connectToServer(std::string password) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_sPort);
    inet_pton(AF_INET, _sAddress.c_str(), &serverAddr.sin_addr);
    if (connect(getFd(), (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
    {
        perror("connect failed: ");
        return;
    }
    std::string passCmd = "PASS " + password + "\r\n";
    send(getFd(), passCmd.c_str(), passCmd.size(), 0);
    send(getFd(), "NICK bot\r\n", 15, 0);
    send(getFd(), "USER bot 0 * :bot\r\n", 21, 0);
    std::cout << "Connected to server at " << _sAddress << ":" << _sPort << std::endl;
}


