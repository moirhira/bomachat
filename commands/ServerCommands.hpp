#ifndef SERVERCOMMANDS_HPP
#define SERVERCOMMANDS_HPP

#include <string>
#include <vector>

class Client;
class Channel;
class Server;

static std::vector<std::string> splitCommaList(const std::string& list);
void sendReply(Client *client, int errorCode, std::string errorMsg, std::string cmd);
static void sendWelcome(Client *client);
bool isPreRegistrationCommand(const std::string &cmd);

bool isPreRegistrationCommand(const std::string &cmd);
void sendReply(Client *client, int errorCode, std::string errorMsg, std::string cmd);
void handlePass(Client *client, std::vector<std::string> params, std::string password);
void handleNick(Client *client, std::vector<std::string> &params, std::vector<Client *> &clients, std::vector<Channel> &channels);
void handleUser(Client *client, const std::vector<std::string> params);
void handleJoin(Client *client, std::vector<std::string> params, std::vector<Channel> &channels);
void handlePrivmsg(Client *client, std::vector<std::string> params, std::vector<Client *> &clients, std::vector<Channel> &channels);
void handleTopic(Client *client, std::vector<std::string> params, std::vector<Channel> &channels);
void handleMode(Client *client, std::vector<std::string> params, std::vector<Channel> &channels);
void handleKick(Client *client, std::vector<std::string> params, std::vector<Channel> &channels);
void handleInvite(Client *client, std::vector<std::string> params, std::vector<Channel> &channels, std::vector<Client *> &clients);
void handlePart(Client *client, std::vector<std::string> params, std::vector<Channel> &channels);
void handleQuit(Client *client, std::vector<std::string> params, std::vector<Channel> &channels, int& i, Server* server);

#endif
