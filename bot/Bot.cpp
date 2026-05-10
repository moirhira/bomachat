#include "Bot.hpp"
#include <cstring>


Bot::Bot(int port, std::string address) : _fd(-1),_sPort(port), _sAddress(address) {}

Bot::~Bot() {
    if (_fd >= 0)
        close(_fd);
}


int Bot::getServerPort() {
    return _sPort;
}

std::string  Bot::getServerAdr() {
    return _sAddress;
}



int Bot::init(std::string nickName, std::string userName, std::string realName) {
    _nickname = nickName;
    _username = userName;
    _realname = realName;

    int _sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_sockfd < 0)
    {
        perror("socket fail: ");
        return 0;
    }
    _fd = _sockfd;
    return 1;
}


void Bot::sendMessge(const std::string &msg) {
    _outBuffer += msg;
}


bool Bot::flushSendBuffer(struct pollfd &pfd) {
    while (!_outBuffer.empty())
    {
        pfd.events = POLLIN | POLLOUT;
        int ret = poll(&pfd, 1, 3000);
        if ( ret < 0)
        {
            perror("poll faild :");
            return false;
        }
        if (ret == 0)
            continue;
        if (!(pfd.revents & POLLOUT))
            break;

        ssize_t sent = send(_fd, _outBuffer.c_str(), _outBuffer.size(), 0);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("send");
            return false;
        }
        _outBuffer.erase(0, static_cast<size_t>(sent));
    }
    return true;
}



bool Bot::doRegistration() {

}



int Bot::connectToServer(std::string password) {
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_sPort);
    inet_pton(AF_INET, _sAddress.c_str(), &serverAddr.sin_addr);

    int ret = connect(_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if ( ret < 0 && errno != EINPROGRESS)
    {
        perror("connect failed: ");
        return false;
    }

    struct pollfd pfd;
    pfd.fd      = _fd;
    pfd.events  = POLLOUT;

    int pollRet = poll(&pfd, 1, 10000);
    if (pollRet <= 0)
    {
        std::cerr << "Connection timed out or poll error." << std::endl;
        return false;
    }



    sendMessge("PASS " + password + "\r\n");
    sendMessge("NICK " + _nickname + "\r\n");
    sendMessge("USER " + _username + " 0 * :" + _realname + "\r\n");


    if (!flushSendBuffer(pfd))
        return false;

    return doRegistration();
    // std::string passCmd = "PASS " + password + "\r\n";
    // send(_fd, passCmd.c_str(), passCmd.size(), 0);

    // std::string nickCmd = "NICK " + _nickname + "\r\n";
    // send(_fd, nickCmd.c_str(), nickCmd.size(), 0);

    // send(_fd, "USER bot 0 * :bot\r\n", strlen("USER bot 0 * :bot\r\n"), 0);

    char buffer[1024];

    while (true)
    {
        size_t n = recv(_fd, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0)
            break;
        
        buffer[n] = '\0';
        std::string data(buffer);

        if (data.find("433") != std::string::npos)
        {
            _nickname = _nickname + "_";
            nickCmd = "NICK " + _nickname + "\r\n";
            send(_fd, nickCmd.c_str(), nickCmd.size(), 0);
        }
        else if (data.find("001") != std::string::npos)
        {
            std::cout << "Registred succesfully" << std::endl;
            std::string joinCmd = "JOIN #general\r\n";
            send(_fd, joinCmd.c_str(), joinCmd.size(), 0);
            return 1;
        }
        else
        {
            break;
        }
    }
    close(_fd);
    std::cerr << "Failed to connect or register with the server." << std::endl;
    return 0;
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

void Bot::handlePrivmsg(const command &cmd) {
    std::string senderNick = cmd.prefix.substr(0, cmd.prefix.find("!"));
    if (senderNick.empty())
        return;
    
    _seenMap[senderNick] = std::time(NULL);
    std::string target = cmd.params[0];
    std::string targetReply = (target[0] == '#') ? target : senderNick;

    std::string msgCommand = cmd.params[1];
    std::istringstream iss(msgCommand);
    std::string commandName;
    iss >> commandName;

    if (commandName == "!help" || commandName == "!time" || commandName == "!roll")
    {
        std::string extra;
        if (iss >> extra)
        {
            std::string errorMsg = "PRIVMSG " + targetReply + " :This command does not take any parameters.\r\n";
            send(_fd, errorMsg.c_str(), errorMsg.size(), 0);
            return;
        }
    }
    if (commandName == "!help")
    {
        std::string helpMsg = "PRIVMSG " + targetReply + " :" + "Available commands: !help !time !roll !seen\r\n";
        send(_fd, helpMsg.c_str(), helpMsg.size(), 0);
    }
    if (commandName == "!time")
    {
        std::time_t now = std::time(NULL);
        std::string timeStr = std::ctime(&now);
        timeStr.erase(timeStr.find("\n"));
        std::string timeMsg = "PRIVMSG " + targetReply + " :Current time: " + timeStr + "\r\n";
        send(_fd, timeMsg.c_str(), timeMsg.size(), 0);
    }
    if (commandName == "!roll")
    {
        int roll = std::rand() % 100 + 1;
        std::ostringstream oss;
        oss << roll;
        std::string rollMsg = "PRIVMSG " + targetReply + " :You rolled a " + oss.str() + "\r\n";
        send(_fd, rollMsg.c_str(), rollMsg.size(), 0);
    }

    if (commandName == "!seen")
    {
        std::string targetNickCheck;
        iss >> targetNickCheck;

        if (targetNickCheck.empty())
        {
            std::string seenMsg = "PRIVMSG " + targetReply + " :Please specify a nickname to check.\r\n";
            send(_fd, seenMsg.c_str(), seenMsg.size(), 0);
            return;

        }

        if(targetNickCheck == senderNick)
        {
            std::string seenMsg = "PRIVMSG " + targetReply + " :That's you!\r\n";
            send(_fd, seenMsg.c_str(), seenMsg.size(), 0);
            return;

        }

        if (_seenMap.find(targetNickCheck) == _seenMap.end())
        {
            std::string seenMsg = "PRIVMSG " + targetReply + " :I haven't seen " + targetNickCheck + "\r\n";
            send(_fd, seenMsg.c_str(), seenMsg.size(), 0);

        }
        else
        {
            std::time_t lastSeen = _seenMap[targetNickCheck];
            std::time_t curTime = std::time(NULL);
            std::time_t dfTime = difftime(curTime, lastSeen);
            std::ostringstream oss;
            oss << dfTime;
            std::string seenMsg = "PRIVMSG " + targetReply + " :Last seen: " + oss.str() + " seconds ago\r\n";
            send(_fd, seenMsg.c_str(), seenMsg.size(), 0);
        }
    }
    
}


void Bot::handelCommand(command cmd)
{
    if (cmd.command.empty())
        return;

    if (cmd.command == "PRIVMSG")
        handlePrivmsg(cmd);
    else if (cmd.command == "KICK")
    {
        if (cmd.params.size() >= 2 && cmd.params[1] == _nickname)
        {
            std::string joinCmd = "JOIN" + cmd.params[0] + "\r\n";
            send(_fd, joinCmd.c_str(), joinCmd.size(), 0);
        }
    }
    else
        return;
}



bool Bot::handleRecv(struct pollfd &pfd) {
    char buffer[512];
    int byts = recv(pfd.fd, buffer, sizeof(buffer) - 1, 0);
    if (byts < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return;
        perror("recv failed: ");
        return false;
    }
    if (byts == 0)
    {
        std::cout << "client disconnected" << std::endl;
        return false;
    }

    _outBuffer.append(buffer, byts);
    if (_buffer.size() > 4096)
    {
        std::cout << "Receive buffer overflow — clearing." << std::endl;
        _outBuffer.clear();
        return true;
    }

        
    size_t pos;
    while ((pos = _buffer.find("\r\n")) != std::string::npos)
    {
        std::string line = _buffer.substr(0, pos);
        _buffer.erase(0, pos + 2);
        command cmd = parseCommand(line);
        handelCommand(cmd);
    }
    return true;
}


void Bot::run() {
    struct pollfd pfd;
    pfd.fd = _fd;

    while (true)
    {
        pfd.events = POLLIN;

        if (!_sendBuffer.empty() || _state == CONNECTING)
            pfd.events |= POLLOUT;

        int ret = poll(&pfd , 1, 5000);
        if ( ret < 0)
        {
            perror("poll");
            break;
        }

        if (ret == 0)
            continue;

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            std::cerr << "Socket error" << std::endl;
            break;
        }

        
        if (pfd.revents & POLLOUT)
        {
            if (!flushSendBuffer(pfd))
                break;
        }

        if (pfd.revents & POLLIN)
        {
            if (!handleRecv(pfd))
                break;
        }
    }
}
