#include "Server.hpp"
#include "../commands/ServerCommands.hpp"

volatile sig_atomic_t running = 1;

void Server::parse_cmd(Client *client, int i, int curFd)
{
	std::string &cmdLine = client->getBuffer();
	size_t pos;
	while ((pos = cmdLine.find("\r\n")) != std::string::npos)
	{
		std::string line = cmdLine.substr(0, pos);
		cmdLine.erase(0, pos + 2);

		command cmd = parseCommand(line);
		handelCommand(cmd, client, i);
		if (!getClientById(curFd))
			return;
	}
}




void Server::Respond_to_client(int i)
{
	Client *client = getClientById(pfds[i].fd);
	if (!client)
		return;
	std::string &buffer = client->getOutBuffer();
	if (buffer.empty())
	{
        return;
    }
	ssize_t n = send(client->getFd(), buffer.c_str(), buffer.length(), 0);
	if (n > 0)
	{
		buffer.erase(0, static_cast<size_t>(n));
	}
	else if (n < 0)
	{
		if (errno == EWOULDBLOCK || errno == EAGAIN)
			return;
		disconnectClient(i);
	}
}

void signalhandler(int sig)
{
	(void)sig;
	running = 0;
}

void Server::handle_events()
{
	int size = this->pfds.size();
	for (int i = size - 1; i >= 0; i--)
	{
		short revents = this->pfds[i].revents;
		int fd = this->pfds[i].fd;
		if (revents & (POLLHUP | POLLERR | POLLNVAL))
		{
			if (fd == this->_sockfd)
				throw std::runtime_error("Server socket error");
			disconnectClient(i);
		}
		else if (revents & POLLIN)
		{
			if (fd == this->_sockfd)
				Accept_client();
			else
				Receive_input(i);
		}
		else if (revents & POLLOUT)
			Respond_to_client(i);
	}
}
void Server::run()
{
	struct pollfd server_pfd;
	server_pfd.fd = this->_sockfd;
	server_pfd.events = POLLIN;
	this->pfds.push_back(server_pfd);
	int ret(-1);
	signal(SIGINT, signalhandler);
	signal(SIGQUIT, signalhandler);
	while (running)
	{
		for (size_t i = 1; i < this->pfds.size(); i++)
		{
			Client *client = getClientById(this->pfds[i].fd);
			if (client)
				this->pfds[i].events = client->getClientEvents();
		}
		ret = poll(&(this->pfds[0]), this->pfds.size(), 3000);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Problem in Poll!");
		}
		else if (ret > 0)
			handle_events();
	}
}

Server::Server(int port1, std::string pass) : _port(port1), _password(pass), _sockfd(-1)
{
	std::cout << "Server Created\nPort : " << this->_port << "\nPassword : " << this->_password << std::endl;
}

void Server::set_port(int p)
{
	this->_port = p;
}

void Server::set_password(std::string &pass)
{
	this->_password = pass;
}

int Server::get_port() const
{
	return (this->_port);
}

const std::string &Server::get_password()
{
	return (this->_password);
}

int Server::get_server_fd()
{
	return (this->_sockfd);
}

Server::~Server()
{
	close(this->_sockfd);
	for (size_t i = 0; i < _clients.size(); i++)
        delete _clients[i];
}
