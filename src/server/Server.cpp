#include "Server.hpp"
#include "../../utils/Metrics.hpp"
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
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		command cmd = parseCommand(line);
		handelCommand(cmd, client, i);
		if (!getClientById(curFd))
			return;
	}
}

void Server::Receive_input(int i)
{
	char buffer[1024] = {0};
	ssize_t n = recv(this->pfds[i].fd, buffer, sizeof(buffer) - 1, 0);
    if (n == 0)
		removeClient(this->pfds[i].fd);
	else if (n < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			removeClient(this->pfds[i].fd);
	}
	else
	{
		Client *tmp = getClientById(pfds[i].fd);
		if (tmp)
		{
			tmp->appendToBuffer(std::string(buffer, n));
			if (tmp->getBuffer().size() > 512)
			{
            	disconnectClient(i);
            	std::cout << "client disconnected (buffer overflow)" << std::endl;
            	return;
        	}
			parse_cmd(tmp, i, pfds[i].fd);
		}
	}
}


void Server::Respond_to_client(int i)
{
	Client *client = getClientById(pfds[i].fd);
	if (!client)
		return;
	std::string &buffer = client->getOutBuffer();
	if (buffer.empty())
		return;
	ssize_t n = send(client->getFd(), buffer.c_str(), buffer.length(), 0);
	if (n > 0)
		buffer.erase(0, static_cast<size_t>(n));
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
		Metrics::instance().setClient(_clients.size());
		Metrics::instance().setChannels(_channels.size());
		ret = poll(&(this->pfds[0]), this->pfds.size(), 3000);
		if (ret < 0)
		{
			if (errno == EINTR)
				continue;
			throw std::runtime_error("Problem in Poll!");
		}
		else if (ret > 0)
		{
			for (int i = this->pfds.size() - 1; i >= 0; i--)
			{
				if (this->pfds[i].revents & POLLIN)
				{
					if (this->pfds[i].fd == this->_sockfd)
						Accept_client();
					else
						Receive_input(i);
				}
				else if (this->pfds[i].revents & POLLOUT)
					Respond_to_client(i);
				else if (this->pfds[i].revents & (POLLHUP | POLLERR | POLLNVAL))
					removeClient(this->pfds[i].fd);
			}
		}
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
