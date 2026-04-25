#include "ServerCommands.hpp"
#include "Server.hpp"

void handleQuit(Client *client, std::vector<std::string> params, std::vector<Channel> &channels, int& i, Server *server) {
    std::string reason;
    if (params.size() > 0)
        reason = params[0];
    else
        reason = "Client Quit";
    std::string quirtMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :" + reason + "\r\n";
    for (size_t j = 0; j < channels.size(); j++)
    {
        if (!channels[j].isMember(client))
            continue;
        channels[j].brodcastMessage(quirtMsg, client);
        channels[j].removeClientEverywhere(client);
    }
    server->disconnectClient(i);
}