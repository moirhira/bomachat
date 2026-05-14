#include "ServerCommands.hpp"
#include "Server.hpp"

void sendReply(Client *client, std::string Code, std::string Msg, std::string cmd)
{
    std::string nick = client->getNickname().empty() ? "*" : client->getNickname();
    std::string commandPart = cmd.empty() ? "" : " " + cmd;
    client->sendMessage(":server " + Code + " " + nick + commandPart + " :" + Msg + "\r\n");
}

void sendWelcome(Client *client)
{
    sendReply(client, "001", "Welcome to the IRC server " + client->getNickname(), "");
    sendReply(client, "002", "Your host is ircserv", "");
    sendReply(client, "003", "This server was created today", "");
    sendReply(client, "004", "ircserv", "");
}

bool isPreRegistrationCommand(const std::string &cmd)
{
    return (cmd == "PASS" || cmd == "NICK" || cmd == "USER" || cmd == "CAP" || cmd == "PING" || cmd == "PONG" || cmd == "QUIT");
}

std::vector<std::string> splitCommaList(const std::string& list)
{
    std::vector<std::string> items;
    size_t start = 0;
    while (start <= list.size())
    {
        size_t commaPos = list.find(',', start);
        size_t len = (commaPos == std::string::npos) ? list.size() - start : commaPos - start;
        items.push_back(list.substr(start, len));
        if (commaPos == std::string::npos)
            break;
        start = commaPos + 1;
    }
    return items;
}

void Server::handelCommand(command cmd, Client *client, int& i)
{
    if (cmd.command.empty())
        return;

    if (!client->isReg() && !isPreRegistrationCommand(cmd.command))
    {
        sendReply(client, "451", "You have not registered", cmd.command);
        return;
    }

    if (cmd.command == "CAP")
    {
        if (cmd.params.empty())
            return;
        std::string sub = cmd.params[0];
        if (sub == "LS")
        {
            std::string reply = ":server CAP * LS :\r\n";
            client->sendMessage(reply);
        }
        else if (sub == "REQ" && cmd.params.size() > 1)
        {
            std::string reply = ":server CAP * NAK :" + cmd.params[1] + "\r\n";
            client->sendMessage(reply);
        }
    }
    else if (cmd.command == "PING")
    {
        std::string token;
        if (!cmd.params.empty())
            token = cmd.params[0];
        std::string reply = ":server PONG server :" + token + "\r\n";
        client->sendMessage(reply);
    }
    else if (cmd.command == "PONG")
        return;
    else if (cmd.command == "PASS")
        handlePass(client, cmd.params, _password);
    else if (cmd.command == "NICK")
        handleNick(client, cmd.params, _clients, _channels);
    else if (cmd.command == "USER")
        handleUser(client, cmd.params);
    else if (cmd.command == "JOIN")
        handleJoin(client, cmd.params, _channels);
    else if (cmd.command == "PRIVMSG")
        handlePrivmsg(client, cmd.params, _clients, _channels);
    else if (cmd.command == "TOPIC")
        handleTopic(client, cmd.params, _channels);
    else if (cmd.command == "MODE")
        handleMode(client, cmd.params, _channels);
    else if (cmd.command == "KICK")
        handleKick(client, cmd.params, _channels);
    else if (cmd.command == "INVITE")
        handleInvite(client, cmd.params, _channels, _clients);
    else if (cmd.command == "PART")
        handlePart(client, cmd.params,  _channels);
    else if (cmd.command == "QUIT")
        handleQuit(client, cmd.params, _channels, i, this);
    else
    {
        sendReply(client, "421", "Unknown command", cmd.command);
    }
}
