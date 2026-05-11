#include "Bot.hpp"

// ─────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────

Bot::Bot(int port, const std::string &address)
    : _fd(-1), _sPort(port), _sAddress(address) {}

Bot::~Bot()
{
    if (_fd >= 0)
        close(_fd);
}

// ─────────────────────────────────────────────
// init — create socket, set O_NONBLOCK, store credentials
// ─────────────────────────────────────────────

bool Bot::init(const std::string &nickName,
               const std::string &userName,
               const std::string &realName,
               const std::string &password)
{
    _nickname = nickName;
    _username = userName;
    _realname = realName;
    _password = password;

    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
    {
        perror("socket");
        return false;
    }

    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0)
    {
        perror("fcntl O_NONBLOCK");
        close(_fd);
        _fd = -1;
        return false;
    }
    return true;
}

// ─────────────────────────────────────────────
// queueSend — append message to outgoing buffer
// ─────────────────────────────────────────────

void Bot::queueSend(const std::string &msg)
{
    _sendBuffer += msg;
}

// ─────────────────────────────────────────────
// flushSendBuffer — drain outgoing buffer without blocking
// Called only when poll() reports POLLOUT.
// Returns false on fatal send error.
// ─────────────────────────────────────────────

bool Bot::flushSendBuffer(struct pollfd &pfd)
{
    (void)pfd;
    while (!_sendBuffer.empty())
    {
        ssize_t sent = send(_fd, _sendBuffer.c_str(), _sendBuffer.size(), 0);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;  // kernel buffer full — will retry when POLLOUT fires again
            perror("send");
            return false;
        }
        _sendBuffer.erase(0, static_cast<size_t>(sent));
    }
    return true;
}

// ─────────────────────────────────────────────
// handleRecv — read data, split into lines, dispatch by state
// Returns false on disconnect or fatal error.
// ─────────────────────────────────────────────

bool Bot::handleRecv(State &state)
{
    char buf[512];
    ssize_t n = recv(_fd, buf, sizeof(buf) - 1, 0);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;    // no data right now — not an error
        perror("recv");
        return false;
    }
    if (n == 0)
    {
        std::cout << "Server disconnected." << std::endl;
        return false;
    }

    _recvBuffer.append(buf, static_cast<size_t>(n));

    // Safety valve — drop buffer if it grows without a valid line
    if (_recvBuffer.size() > 4096)
    {
        std::cerr << "Receive buffer overflow — clearing." << std::endl;
        _recvBuffer.clear();
        return true;
    }

    size_t pos;
    while ((pos = _recvBuffer.find("\r\n")) != std::string::npos)
    {
        std::string line = _recvBuffer.substr(0, pos);
        _recvBuffer.erase(0, pos + 2);

        if (state == REGISTERING)
        {
            // 001 — welcome, registration complete
            if (line.find(" 001 ") != std::string::npos)
            {
                std::cout << "Registered successfully as " << _nickname << std::endl;
                queueSend("JOIN #general\r\n");
                state = RUNNING;
            }
            // 433 — nickname already in use
            else if (line.find(" 433 ") != std::string::npos)
            {
                _nickname += "_";
                queueSend("NICK " + _nickname + "\r\n");
            }
            // PING during registration (some servers send it early)
            else if (line.size() >= 4 && line.substr(0, 4) == "PING")
            {
                std::string token = (line.size() > 6) ? line.substr(5) : "";
                queueSend("PONG :" + token + "\r\n");
            }
        }
        else if (state == RUNNING)
        {
            Command cmd = parseCommand(line);
            handleCommand(cmd);
        }
    }
    return true;
}

// ─────────────────────────────────────────────
// parseCommand — parse a single IRC line
// ─────────────────────────────────────────────

Command Bot::parseCommand(const std::string &cmdLine)
{
    Command cmd;
    if (cmdLine.empty())
        return cmd;

    std::string s = cmdLine;

    // Strip leading whitespace
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
        return cmd;
    s = s.substr(start);

    // Optional prefix (:nick!user@host)
    if (s[0] == ':')
    {
        size_t sp = s.find(' ');
        if (sp == std::string::npos)
            return cmd;
        cmd.prefix = s.substr(1, sp - 1);
        s = s.substr(sp + 1);
    }

    // Command name
    size_t sp = s.find(' ');
    cmd.name = s.substr(0, sp);
    for (size_t i = 0; i < cmd.name.size(); i++)
        cmd.name[i] = static_cast<char>(toupper(static_cast<unsigned char>(cmd.name[i])));

    if (sp == std::string::npos)
        return cmd;
    s.erase(0, sp + 1);

    // Parameters
    while (!s.empty())
    {
        if (s[0] == ':')
        {
            cmd.params.push_back(s.substr(1));
            break;
        }
        size_t pos = s.find(' ');
        if (pos == std::string::npos)
        {
            cmd.params.push_back(s);
            break;
        }
        cmd.params.push_back(s.substr(0, pos));
        s.erase(0, pos + 1);
    }
    return cmd;
}

// ─────────────────────────────────────────────
// handlePrivmsg — respond to bot commands
// ─────────────────────────────────────────────

void Bot::handlePrivmsg(const Command &cmd)
{
    if (cmd.params.size() < 2)
        return;

    std::string senderNick = cmd.prefix.substr(0, cmd.prefix.find('!'));
    if (senderNick.empty())
        return;

    _seenMap[senderNick] = std::time(NULL);

    const std::string &target = cmd.params[0];
    std::string replyTarget = (!target.empty() && target[0] == '#') ? target : senderNick;

    std::istringstream iss(cmd.params[1]);
    std::string commandName;
    iss >> commandName;

    // Commands that take no extra arguments
    if (commandName == "!help" || commandName == "!time" || commandName == "!roll")
    {
        std::string extra;
        if (iss >> extra)
        {
            queueSend("PRIVMSG " + replyTarget +
                      " :This command does not take any parameters.\r\n");
            return;
        }
    }

    if (commandName == "!help")
    {
        queueSend("PRIVMSG " + replyTarget +
                  " :Available commands: !help !time !roll !seen\r\n");
    }
    else if (commandName == "!time")
    {
        std::time_t now = std::time(NULL);
        std::string timeStr = std::ctime(&now);
        if (!timeStr.empty() && timeStr[timeStr.size() - 1] == '\n')
            timeStr.erase(timeStr.size() - 1);
        queueSend("PRIVMSG " + replyTarget + " :Current time: " + timeStr + "\r\n");
    }
    else if (commandName == "!roll")
    {
        int roll = std::rand() % 100 + 1;
        std::ostringstream oss;
        oss << roll;
        queueSend("PRIVMSG " + replyTarget + " :You rolled a " + oss.str() + "\r\n");
    }
    else if (commandName == "!seen")
    {
        std::string targetNick;
        iss >> targetNick;

        if (targetNick.empty())
        {
            queueSend("PRIVMSG " + replyTarget + " :Usage: !seen <nickname>\r\n");
            return;
        }
        if (targetNick == senderNick)
        {
            queueSend("PRIVMSG " + replyTarget + " :That's you!\r\n");
            return;
        }
        if (_seenMap.find(targetNick) == _seenMap.end())
        {
            queueSend("PRIVMSG " + replyTarget +
                      " :I haven't seen " + targetNick + "\r\n");
        }
        else
        {
            std::time_t elapsed = static_cast<std::time_t>(
                difftime(std::time(NULL), _seenMap[targetNick]));
            std::ostringstream oss;
            oss << elapsed;
            queueSend("PRIVMSG " + replyTarget +
                      " :Last seen: " + oss.str() + " seconds ago\r\n");
        }
    }
}

// ─────────────────────────────────────────────
// handleCommand — dispatch parsed command
// ─────────────────────────────────────────────

void Bot::handleCommand(const Command &cmd)
{
    if (cmd.name.empty())
        return;

    if (cmd.name == "PRIVMSG")
        handlePrivmsg(cmd);
    else if (cmd.name == "KICK")
    {
        // params[0] = channel, params[1] = kicked nick
        if (cmd.params.size() >= 2 && cmd.params[1] == _nickname)
            queueSend("JOIN " + cmd.params[0] + "\r\n");
    }
    else if (cmd.name == "PING")
    {
        std::string token = cmd.params.empty() ? "" : cmd.params[0];
        queueSend("PONG :" + token + "\r\n");
    }
}

// ─────────────────────────────────────────────
// run — THE single event loop
//
//  CONNECTING  → wait for non-blocking connect to complete (POLLOUT)
//  REGISTERING → send PASS/NICK/USER, read 001/433 replies
//  RUNNING     → normal bot operation
//
// One poll() call drives everything.
// ─────────────────────────────────────────────

void Bot::run()
{
    // Kick off non-blocking connect
    struct sockaddr_in serverAddr;
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(_sPort);
    serverAddr.sin_addr.s_addr = inet_addr(_sAddress.c_str());
    if (serverAddr.sin_addr.s_addr == (in_addr_t)(-1))
    {
        std::cerr << "Invalid address: " << _sAddress << std::endl;
        return;
    }

    int ret = connect(_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0 && errno != EINPROGRESS)
    {
        perror("connect");
        return;
    }

    State state = (ret == 0) ? REGISTERING : CONNECTING;

    // If connect completed instantly (loopback), queue registration right away
    if (state == REGISTERING)
    {
        queueSend("PASS " + _password + "\r\n");
        queueSend("NICK " + _nickname + "\r\n");
        queueSend("USER " + _username + " 0 * :" + _realname + "\r\n");
    }

    struct pollfd pfd;
    pfd.fd = _fd;

    while (true)
    {
        // Always listen for incoming data; also POLLOUT when we have data to send
        // or are still waiting for connect to complete
        pfd.events = POLLIN;
        if (!_sendBuffer.empty() || state == CONNECTING)
            pfd.events |= POLLOUT;

        int pollRet = poll(&pfd, 1, 10000);
        if (pollRet < 0)
        {
            perror("poll");
            break;
        }
        if (pollRet == 0)
        {
            // Timeout — only fatal during connection/registration
            if (state != RUNNING)
            {
                std::cerr << "Timed out waiting for server." << std::endl;
                break;
            }
            continue;
        }

        // ── WRITE / CONNECT PATH ─────────────────────
        if (pfd.revents & POLLOUT)
        {
            if (state == CONNECTING)
            {
                // Check whether the non-blocking connect succeeded
                int soErr = 0;
                socklen_t len = sizeof(soErr);
                if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &soErr, &len) < 0 || soErr != 0)
                {
                    std::cerr << "connect failed: " << strerror(soErr) << std::endl;
                    break;
                }
                // Connected — queue registration commands and advance state
                queueSend("PASS " + _password + "\r\n");
                queueSend("NICK " + _nickname + "\r\n");
                queueSend("USER " + _username + " 0 * :" + _realname + "\r\n");
                state = REGISTERING;
            }

            if (!flushSendBuffer(pfd))
                break;
        }

        // ── READ PATH ────────────────────────────────
        if (pfd.revents & POLLIN)
        {
            if (!handleRecv(state))
                break;
        }
    }
}