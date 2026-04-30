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
    _nickname = nickName;
    _username = userName;
    _realname = realName;

    int _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        perror("socket fail: ");
        return;
    }
    _fd = _sockfd;
};

void Bot::connectToServer(std::string password) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_sPort);
    inet_pton(AF_INET, _sAddress.c_str(), &serverAddr.sin_addr);
    if (connect(_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0 )
    {
        perror("connect failed: ");
        return;
    }
    std::string passCmd = "PASS " + password + "\r\n";
    send(_fd, passCmd.c_str(), passCmd.size(), 0);
    send(_fd, "NICK bot\r\n", 15, 0);
    send(_fd, "USER bot 0 * :bot\r\n", 21, 0);

    char buffer[1024];

    while (true)
    {
        size_t n = recv(_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
            break;
        
        buffer[n] = '\0';
        std::string data(buffer);

        if (data.find("001") != std::string::npos)
        {
            std::cout << "Registred succesfully" << std::endl;
            return;
        }
    }
    recv(_fd, buffer, 1024, 0);
    std::cout << "Received from server: " << buffer << std::endl;




    std::cout << "Connected to server at " << _sAddress << ":" << _sPort << std::endl;
}


