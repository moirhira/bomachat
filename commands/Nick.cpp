#include "ServerCommands.hpp"
#include "Server.hpp"
#include <cctype>
#include <set>

static bool isValidNickChar(char c)
{
    return isalnum(c) || c == '-' || c == '_'
        || c == '[' || c == ']' || c == '\\'
        || c == '^' || c == '{' || c == '}'
        || c == '|';
}

static bool isValidNick(const std::string &nick)
{
    if (nick.empty())
        return false;
    if (nick.size() > 9 || isdigit(nick[0]) || nick[0] == '-')
        return false;
    for (size_t i = 0; i < nick.size(); i++)
    {
        if (!isValidNickChar(nick[i]))
            return false;
    }
    return true;
}

Client *findClientByNick(std::vector<Client *> &clients, const std::string &nick, Client *exclude)
{
    for (size_t i = 0; i < clients.size(); i++)
    {
        if (clients[i] != exclude && clients[i]->getNickname() == nick)
            return clients[i];
    }
    return NULL;
}

void handleNick(Client *client, std::vector<std::string>& params,
                std::vector<Client *> &clients, std::vector<Channel> &channels)
{
    if (!client->isAuth())
        return sendReply(client, "451", "You have not registered", "NICK");
    if (params.empty() || params[0].empty())
        return sendReply(client, "431", "No nickname given", "NICK");

    std::string newNick = params[0];

    if (!isValidNick(newNick))
        return sendReply(client, "432", "Erroneous nickname", newNick);

    if (findClientByNick(clients, newNick, client))
        return sendReply(client, "433", "Nickname is already in use", newNick);

    bool wasReg = client->isReg();
    std::string oldNick = client->getNickname();

    client->setNickname(newNick);
    if (!client->getUsername().empty())
        client->setRegistered(true);

    if (client->isReg() && !wasReg)
    {
        sendWelcome(client);
    }
    else if (wasReg)
    {
        std::string nickMsg = ":" + oldNick + "!" + client->getUsername()
            + "@localhost NICK :" + newNick + "\r\n";

        client->sendMessage(nickMsg);
        std::set<Client*> notified;
        for (size_t i = 0; i < channels.size(); i++)
        {
            if (!channels[i].isMember(client))
                continue;
            std::vector<Client*> members = channels[i].getMembers();
            for (size_t j = 0; j < members.size(); j++)
            {
                if (members[j] != client && notified.find(members[j]) == notified.end())
                {
                    members[j]->sendMessage(nickMsg);
                    notified.insert(members[j]);
                }
            }
        }
    }
}
