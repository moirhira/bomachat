#include "Bot.hpp"


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