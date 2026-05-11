#include "Bot.hpp"
#include <cstring>
#include <fcntl.h>
#include <csignal>
volatile sig_atomic_t g_running = 1;


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

    while (g_running)
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
