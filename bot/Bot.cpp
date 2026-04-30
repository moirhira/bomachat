#include "Bot.hpp"
#include <cstring>


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
    send(_fd, "NICK bot\r\n", strlen("NICK bot\r\n"), 0);
    send(_fd, "USER bot 0 * :bot\r\n", strlen("USER bot 0 * :bot\r\n"), 0);

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
            std::string joinCmd = "JOIN #general\r\n";
            send(_fd, joinCmd.c_str(), joinCmd.size(), 0);
            return;
            return;
        }
    }
}



command Bot::parseCommand(std::string cmdLine)
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
        cmdStruct.prefix = trimmedCmd.substr(1, spacePos - 1);
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


void Bot::handleJoin()
{

}
void Bot::handlePrivmsg(const command &cmd) {
    std::string senderNick = cmd.prefix.substr(0, cmd.prefix.find("!"));
    if (senderNick.empty())
        return;
    
    std::string target = cmd.params[0];
    std::string msgCommand = cmd.params[1];

    std::cout << "sender ->  " << senderNick << std::endl;
    std::cout << "target ->  " << target << std::endl;
    std::cout << "message ->  " << msgCommand << std::endl;
    std::string targetReply = (target[0] == '#') ? target : senderNick;
    

    if (msgCommand == "!help")
    {
        std::string helpMsg = "PRIVMSG " + targetReply + " :" + "Available commands: !help !time !roll\r\n";
        send(_fd, helpMsg.c_str(), helpMsg.size(), 0);
    }

    
}

void Bot::handleError() {

}

void Bot::handleKick() {

}

void Bot::handelCommand(command cmd)
{
    if (cmd.command.empty())
        return;

    if (cmd.command == "JOIN")
        handleJoin();
    else if (cmd.command == "PRIVMSG")
        handlePrivmsg(cmd);
    else if (cmd.command == "KICK")
        handleKick();
    else if (cmd.command == "ERROR")
        handleError();
    else
    {
        std::cout << "Unknown command: " << cmd.command << std::endl;
    }
}

void Bot::run() {
    pollfd fds[1];
    fds[0].fd = _fd;
    fds[0].events = POLLIN;
    while (true)
    {
        if (poll(fds , 1, 500) < 0)
        {
            perror("poll failed: ");
            break;
        }
        if (fds[0].revents & POLLIN)
        {
            char buffer[1024];
            int byts = recv(fds[0].fd, buffer, sizeof(buffer) - 1, 0);
            if (byts < 0)
            {
                if (errno == EWOULDBLOCK || errno == EAGAIN)
                    return;
                perror("recv failed: ");
                return;
            }
            if (byts == 0)
            {
                close(_fd);
                std::cout << "client disconnected" << std::endl;
            }
            else
            {
                buffer[byts] = '\0';
                _buffer.append(buffer, byts);
                if (_buffer.size() > 4096)
                {
                    close(_fd);
                    std::cout << "client disconnected (buffer overflow)" << std::endl;
                    return;
                }
                size_t pos;
                while ((pos = _buffer.find("\r\n")) != std::string::npos)
                {
                    std::string line = _buffer.substr(0, pos);
                    _buffer.erase(0, pos + 2);
                    if (!line.empty() && line[line.size() - 1] == '\r')
                        line.erase(line.size() - 1);
                    std::cout << "Received: " << line << std::endl;
                    command cmd = parseCommand(line);

                    handelCommand(cmd);

                    // if (!getClientById(curFd))
                    //     return;
                }
            }

        }


    }
}
