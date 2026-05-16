#include "Bot.hpp"


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