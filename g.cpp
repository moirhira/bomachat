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
// Getters
// ─────────────────────────────────────────────

int Bot::getServerPort() { return _sPort; }
std::string Bot::getServerAddress() { return _sAddress; }

// ─────────────────────────────────────────────
// init — create socket and set O_NONBLOCK
// ─────────────────────────────────────────────

bool Bot::init(const std::string &nickName,
               const std::string &userName,
               const std::string &realName)
{
    _nickname = nickName;
    _username = userName;
    _realname = realName;

    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd < 0)
    {
        perror("socket");
        return false;
    }

    // Set socket non-blocking — mandatory per subject
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
// queueSend — append to outgoing buffer
// ─────────────────────────────────────────────

void Bot::queueSend(const std::string &msg)
{
    _sendBuffer += msg;
}

// ─────────────────────────────────────────────
// flushSendBuffer — drain as much as possible via poll POLLOUT
// Returns false on fatal error.
// ─────────────────────────────────────────────

bool Bot::flushSendBuffer(struct pollfd &pfd)
{
    while (!_sendBuffer.empty())
    {
        // Wait until the socket is writable
        pfd.events = POLLIN | POLLOUT;
        int ret = poll(&pfd, 1, 3000);
        if (ret < 0)
        {
            perror("poll (flush)");
            return false;
        }
        if (ret == 0)          // timeout — try again
            continue;
        if (!(pfd.revents & POLLOUT))
            break;             // not writable yet, leave data in buffer

        ssize_t sent = send(_fd, _sendBuffer.c_str(), _sendBuffer.size(), 0);
        if (sent < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                break;         // kernel buffer full — try next iteration
            perror("send");
            return false;
        }
        _sendBuffer.erase(0, static_cast<size_t>(sent));
    }
    return true;
}

// ─────────────────────────────────────────────
// connectToServer — non-blocking connect + registration
// ─────────────────────────────────────────────

bool Bot::connectToServer(const std::string &password)
{
    struct sockaddr_in serverAddr;
    serverAddr.sin_family      = AF_INET;
    serverAddr.sin_port        = htons(_sPort);
    // inet_addr is in the allowed list; inet_pton is not
    serverAddr.sin_addr.s_addr = inet_addr(_sAddress.c_str());
    if (serverAddr.sin_addr.s_addr == (in_addr_t)(-1))
    {
        std::cerr << "Invalid address: " << _sAddress << std::endl;
        return false;
    }

    // Non-blocking connect: returns immediately with EINPROGRESS
    int ret = connect(_fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
    if (ret < 0 && errno != EINPROGRESS)
    {
        perror("connect");
        return false;
    }

    // Wait for the connection to complete via POLLOUT
    struct pollfd pfd;
    pfd.fd     = _fd;
    pfd.events = POLLOUT;

    int pollRet = poll(&pfd, 1, 10000); // 10 s timeout
    if (pollRet <= 0)
    {
        std::cerr << "Connection timed out or poll error." << std::endl;
        return false;
    }

    // Check SO_ERROR to confirm the connection succeeded
    int soErr = 0;
    socklen_t len = sizeof(soErr);
    if (getsockopt(_fd, SOL_SOCKET, SO_ERROR, &soErr, &len) < 0 || soErr != 0)
    {
        std::cerr << "connect SO_ERROR: " << strerror(soErr) << std::endl;
        return false;
    }

    // Queue registration commands — they will be flushed through poll
    queueSend("PASS " + password + "\r\n");
    queueSend("NICK " + _nickname + "\r\n");
    queueSend("USER " + _username + " 0 * :" + _realname + "\r\n");

    if (!flushSendBuffer(pfd))
        return false;

    // Now drive the registration reply loop through poll
    return doRegistration();
}

// ─────────────────────────────────────────────
// doRegistration — read server replies until 001 (welcome)
// All I/O goes through poll, socket stays non-blocking throughout.
// ─────────────────────────────────────────────

bool Bot::doRegistration()
{
    struct pollfd pfd;
    pfd.fd     = _fd;
    pfd.events = POLLIN;

    while (true)
    {
        int ret = poll(&pfd, 1, 10000);
        if (ret < 0)
        {
            perror("poll (registration)");
            return false;
        }
        if (ret == 0)
        {
            std::cerr << "Registration timed out." << std::endl;
            return false;
        }

        if (pfd.revents & POLLIN)
        {
            char buf[512];
            ssize_t n = recv(_fd, buf, sizeof(buf) - 1, 0);
            if (n < 0)
            {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                    continue;
                perror("recv (registration)");
                return false;
            }
            if (n == 0)
            {
                std::cerr << "Server closed connection during registration." << std::endl;
                return false;
            }

            _recvBuffer.append(buf, static_cast<size_t>(n));

            // Process all complete lines
            size_t pos;
            while ((pos = _recvBuffer.find("\r\n")) != std::string::npos)
            {
                std::string line = _recvBuffer.substr(0, pos);
                _recvBuffer.erase(0, pos + 2);

                // Numeric 433 — nick already in use
                if (line.find(" 433 ") != std::string::npos)
                {
                    _nickname += "_";
                    queueSend("NICK " + _nickname + "\r\n");
                    if (!flushSendBuffer(pfd))
                        return false;
                }
                // Numeric 001 — welcome, registration complete
                else if (line.find(" 001 ") != std::string::npos)
                {
                    std::cout << "Registered successfully as " << _nickname << std::endl;
                    queueSend("JOIN #general\r\n");
                    if (!flushSendBuffer(pfd))
                        return false;
                    return true;
                }
                // PING during registration (servers sometimes send it)
                else if (line.substr(0, 4) == "PING")
                {
                    std::string token = line.substr(5);
                    queueSend("PONG :" + token + "\r\n");
                    if (!flushSendBuffer(pfd))
                        return false;
                }
            }
        }
    }
}

// ─────────────────────────────────────────────
// parseCommand
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
// handlePrivmsg
// ─────────────────────────────────────────────

void Bot::handlePrivmsg(const Command &cmd)
{
    // Need at least: target + message
    if (cmd.params.size() < 2)
        return;

    std::string senderNick = cmd.prefix.substr(0, cmd.prefix.find('!'));
    if (senderNick.empty())
        return;

    _seenMap[senderNick] = std::time(NULL);

    const std::string &target = cmd.params[0];
    // Reply to channel if PRIVMSG was sent to a channel, else reply privately
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
        // ctime appends '\n', remove it
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
            queueSend("PRIVMSG " + replyTarget +
                      " :Usage: !seen <nickname>\r\n");
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
// handleCommand — dispatch
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
        {
            // Fix: there was a missing space → "JOIN#general"
            queueSend("JOIN " + cmd.params[0] + "\r\n");
        }
    }
    else if (cmd.name == "PING")
    {
        // Respond to server PING to avoid timeout
        std::string token = cmd.params.empty() ? "" : cmd.params[0];
        queueSend("PONG :" + token + "\r\n");
    }
}

// ─────────────────────────────────────────────
// handleRecv — read incoming data into _recvBuffer
// Returns false on fatal error / disconnect.
// ─────────────────────────────────────────────

bool Bot::handleRecv(struct pollfd &pfd)
{
    char buf[512];
    ssize_t n = recv(pfd.fd, buf, sizeof(buf) - 1, 0);
    if (n < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;       // no data right now — not an error
        perror("recv");
        return false;
    }
    if (n == 0)
    {
        std::cout << "Server disconnected." << std::endl;
        return false;
    }

    _recvBuffer.append(buf, static_cast<size_t>(n));

    // Guard against a runaway buffer (no valid IRC line in 4 KB → drop)
    if (_recvBuffer.size() > 4096)
    {
        std::cerr << "Receive buffer overflow — clearing." << std::endl;
        _recvBuffer.clear();
        return true;
    }

    // Dispatch every complete line
    size_t pos;
    while ((pos = _recvBuffer.find("\r\n")) != std::string::npos)
    {
        std::string line = _recvBuffer.substr(0, pos);
        _recvBuffer.erase(0, pos + 2);
        Command cmd = parseCommand(line);
        handleCommand(cmd);
    }

    return true;
}

// ─────────────────────────────────────────────
// run — main event loop, single poll(), fully non-blocking
// ─────────────────────────────────────────────

void Bot::run()
{
    struct pollfd pfd;
    pfd.fd     = _fd;
    pfd.events = POLLIN;

    while (true)
    {
        // Ask for POLLOUT only when we have pending data to send
        pfd.events = POLLIN;
        if (!_sendBuffer.empty())
            pfd.events |= POLLOUT;

        int ret = poll(&pfd, 1, 500);
        if (ret < 0)
        {
            perror("poll");
            break;
        }

        // Write path — flush whatever is queued
        if (pfd.revents & POLLOUT)
        {
            if (!flushSendBuffer(pfd))
                break;
        }

        // Read path
        if (pfd.revents & POLLIN)
        {
            if (!handleRecv(pfd))
                break;
        }
    }
}