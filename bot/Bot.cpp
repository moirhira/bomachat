#include "Bot.hpp"
#include <cstring>
#include <fcntl.h>


Bot::Bot(int port, std::string address) : _fd(-1),_sPort(port), _sAddress(address), _state(CONNECTING) {}

Bot::~Bot() {
    if (_fd >= 0)
        close(_fd);
}



bool Bot::init(std::string& nickName, std::string& userName, std::string& realName, std::string& password) {
    _nickname = nickName;
    _username = userName;
    _realname = realName;
    _password = password;

    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
    {
        perror("socket fail: ");
        return 0;
    }
    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl");
        close(_fd);
        _fd = -1;
        return false;
    }
    return true;
}


void Bot::sendMessge(const std::string &msg) {
    _sendBuffer += msg;
}


bool Bot::flushSendBuffer() {
    while (!_sendBuffer.empty())
    {
        ssize_t sent = send(_fd, _sendBuffer.c_str(), _sendBuffer.size(), 0);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;
            perror("send");
            return false;
        }
        _sendBuffer.erase(0, static_cast<size_t>(sent));
    }
    return true;
}


bool Bot::handleRecv() {
    char buffer[512];

    while (true)
    {
        ssize_t byts = recv(_fd, buffer, sizeof(buffer) - 1, 0);
        if (byts < 0)
        {
            if (errno == EWOULDBLOCK || errno == EAGAIN)
                break;
            perror("recv failed: ");
            return false;
        }
        if (byts == 0)
        {
            std::cout << "Server disconnected" << std::endl;
            return false;
        }

        _recvBuffer.append(buffer, static_cast<size_t>(byts));
        if (_recvBuffer.size() > 4096)
        {
            std::cout << "Receive buffer overflow — clearing." << std::endl;
            _recvBuffer.clear();
            return true;
        }
    }

        
    size_t pos;
    while ((pos = _recvBuffer.find("\r\n")) != std::string::npos)
    {
        std::string line = _recvBuffer.substr(0, pos);
        _recvBuffer.erase(0, pos + 2);
        handleLine(line);
    }
    return true;
}


void Bot::handleLine(const std::string& line)
{
    if (_state == REGISTERING)
    {
        if (line.size() >= 4 && line.substr(0, 4) == "PING")
        {
            std::string token = (line.size() > 5) ? line.substr(5) : "";
            sendMessge("PONG :" + token + "\r\n");
        }
        else if (line.find(" 001 ") != std::string::npos)
        {
            std::cout << "Registred succesfully" << std::endl;
            sendMessge("JOIN #general\r\n");
            _state = RUNNING;
        }
        else if (line.find(" 433 ") != std::string::npos)
        {
            _nickname += "_";
            sendMessge("NICK " + _nickname + "\r\n");
        }
    }
    else if (_state == RUNNING)
    {
        command cmd = parseCommand(line);
        handelCommand(cmd);
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




void Bot::handlePrivmsg(const command &cmd) {
    if (cmd.params.size() < 2)
        return;

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
            sendMessge(errorMsg);
            return;
        }
    }
    if (commandName == "!help")
    {
        std::string helpMsg = "PRIVMSG " + targetReply + " :" + "Available commands: !help !time !roll !seen\r\n";
        sendMessge(helpMsg);
    }
    if (commandName == "!time")
    {
        std::time_t now = std::time(NULL);
        std::string timeStr = std::ctime(&now);
        timeStr.erase(timeStr.find("\n"));
        std::string timeMsg = "PRIVMSG " + targetReply + " :Current time: " + timeStr + "\r\n";
        sendMessge(timeMsg);
    }
    if (commandName == "!roll")
    {
        int roll = std::rand() % 100 + 1;
        std::ostringstream oss;
        oss << roll;
        std::string rollMsg = "PRIVMSG " + targetReply + " :You rolled a " + oss.str() + "\r\n";
        sendMessge(rollMsg);
    }

    if (commandName == "!seen")
    {
        std::string targetNickCheck;
        iss >> targetNickCheck;

        if (targetNickCheck.empty())
        {
            std::string seenMsg = "PRIVMSG " + targetReply + " :Please specify a nickname to check.\r\n";
            sendMessge(seenMsg);
            return;

        }

        if(targetNickCheck == senderNick)
        {
            std::string seenMsg = "PRIVMSG " + targetReply + " :That's you!\r\n";
            sendMessge(seenMsg);
            return;

        }

        if (_seenMap.find(targetNickCheck) == _seenMap.end())
        {
            std::string seenMsg = "PRIVMSG " + targetReply + " :I haven't seen " + targetNickCheck + "\r\n";
            sendMessge(seenMsg);
        }
        else
        {
            std::time_t lastSeen = _seenMap[targetNickCheck];
            std::time_t curTime = std::time(NULL);
            std::time_t dfTime = difftime(curTime, lastSeen);
            std::ostringstream oss;
            oss << dfTime;
            std::string seenMsg = "PRIVMSG " + targetReply + " :Last seen: " + oss.str() + " seconds ago\r\n";
            sendMessge(seenMsg);
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
            std::string joinCmd = "JOIN " + cmd.params[0] + "\r\n";
            sendMessge(joinCmd);
        }
    }
    else if (cmd.command == "PING")
    {
        std::string token = cmd.params.empty() ? "" : cmd.params[0];
        sendMessge("PONG :" + token + "\r\n");
    }
}


bool Bot::connectAsync() {
    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(_sPort);

    if (inet_pton(AF_INET, _sAddress.c_str(), &serverAddr.sin_addr) <= 0)
    {
        std::cerr << "Invalid address" << std::endl;
        return false;
    }

    int ret = connect(_fd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr));

    if (ret == 0)
    {
        sendMessge("PASS " + _password + "\r\n");
        sendMessge("NICK " + _nickname + "\r\n");
        sendMessge("USER " + _username + " 0 * :" + _realname + "\r\n"); 
        _state = REGISTERING;
        return true;
    }

    if ( ret < 0 && errno != EINPROGRESS)
    {
        perror("connect failed: ");
        return false;
    }

    _state = CONNECTING;

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
        {
            if (_state != RUNNING)
            {
                std::cerr << "Timed out waiting for server." << std::endl;
                break;
            }
            continue;
        }

        if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
        {
            std::cerr << "Socket error" << std::endl;
            break;
        }


        if (pfd.revents & POLLOUT)
        {
            if (_state == CONNECTING)
            {
                int soError = 0;
                socklen_t len = sizeof(soError);

                getsockopt(_fd, SOL_SOCKET, SO_ERROR, &soError, &len);

                if (soError != 0)
                {
                    std::cerr << "Connect failed" << std::endl;
                    break;
                }
                sendMessge("PASS " + _password + "\r\n");
                sendMessge("NICK " + _nickname + "\r\n");
                sendMessge("USER " + _username + " 0 * :" + _realname + "\r\n"); 
                _state = REGISTERING;

            }
            if (!flushSendBuffer())
                break;
        }

        if (pfd.revents & POLLIN)
        {
            if (!handleRecv())
                break;
        }
    }
}
