#include "server/Server.hpp"
#include <signal.h>


bool is_alldigit(std::string &s)
{
	for (size_t i = 0; i < s.length(); i++)
	{
		if (!isdigit(s[i]))
			return 0;
	}
	return 1;
}

int validate_input(std::string &s)
{
	if (!is_alldigit(s))
	{
		std::cerr << "Invalid port format!" << std::endl;
		return 0;
	}
	int port = atoi(s.c_str());
	if (port <= 0 || port > 65535)
	{
		std::cerr << "Invalid port" << std::endl;
		return 0;
	}
	return port;
}

int main(int ac, char **av)
{
	signal(SIGPIPE, SIG_IGN);
	if (ac != 3)
	{
		std::cerr << "Input format : ./ircserv <port> <password>" << std::endl;
		return 1;
	}
	std::string s(av[1]), pass(av[2]);
	int port = validate_input(s);
	if (!port)
		return 1;
	
	try
	{
		Server server(port, pass);
		server.CreateServerSocket();
		server.run();
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
		return 1;
	}
	return 0;
}
