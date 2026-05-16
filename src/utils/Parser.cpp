#include "ServerCommands.hpp"
#include "Server.hpp"
#include <cctype>

command Server::parseCommand(std::string cmdLine)
{
    command cmdStruct;
    std::string token;
    std::string rest;

    size_t end = cmdLine.find_last_not_of("\r\n");
    if (end == std::string::npos)
        return cmdStruct;
    cmdLine = cmdLine.substr(0, end + 1);

    std::istringstream iss(cmdLine);

    if (cmdLine[0] == ':')
    {
        iss >> token;
        if (iss.eof())
            return cmdStruct;
    }
    if (!(iss >> cmdStruct.command))
        return cmdStruct;
    for (size_t i = 0; i < cmdStruct.command.size(); i++)
        cmdStruct.command[i] = toupper(cmdStruct.command[i]);

    while (iss >> token)
    {
        if (token[0] == ':')
        {
            std::getline(iss, rest);
            cmdStruct.params.push_back(token.substr(1) + rest);
            break;
        }
        cmdStruct.params.push_back(token);
    }
    return cmdStruct;
}