#include "Server.hpp"
#include "Client.hpp"


void Server::CreateServerSocket()
{
	this->_sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_sockfd < 0)
		throw std::runtime_error("Socket creation failed");
	struct sockaddr_in my_addr;
	std::memset(&(my_addr), 0, sizeof(my_addr));
	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(this->_port);
	my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	int opt = 1;
	if (setsockopt(this->_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("Setsockopt failed!");
	if (bind(_sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
		throw std::runtime_error("Binding failed!");
	if (listen(this->_sockfd, 15) < 0)
		throw std::runtime_error("Listening failed!");
	fcntl(this->_sockfd, F_SETFL, O_NONBLOCK);
}


void Server::Accept_client()
{
	int client_fd = accept(this->_sockfd, NULL, NULL);
	if (client_fd < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
        	std::cerr << "accept() failed: " << strerror(errno) << std::endl;
    	return;
	}
	Client *tmp = new Client(client_fd);
	_clients.push_back(tmp);
	struct pollfd client_pfd;
	client_pfd.fd = client_fd;
    client_pfd.events = POLLIN;
	client_pfd.revents = 0;
	this->pfds.push_back(client_pfd);
	std::cout << "New Client Accepted" << std::endl;
	fcntl(client_fd, F_SETFL, O_NONBLOCK);
}


void Server::Receive_input(int i)
{
	char buffer[1024] = {0};
	ssize_t n = recv(this->pfds[i].fd, buffer, sizeof(buffer) - 1, 0);
    if (n == 0)
		return disconnectClient(i);
	if (n < 0)
	{
		if (errno != EAGAIN && errno != EWOULDBLOCK)
			return disconnectClient(i);
	}
	else
	{
		Client *tmp = getClientById(pfds[i].fd);
		if (tmp)
		{
			tmp->appendToBuffer(std::string(buffer, n));
			if (tmp->getBuffer().size() > 512)
			{
            	std::cout << "client disconnected (buffer overflow)" << std::endl;
            	return disconnectClient(i);
        	}
			parse_cmd(tmp, i, pfds[i].fd);
		}
	}
}

Client *Server::getClientById(int fd)
{
	for (size_t i = 0; i < _clients.size(); i++)
	{
		if (_clients[i]->getFd() == fd)
			return _clients[i];
	}
	return NULL;
}

void Server::disconnectClient(int i)
{
    int fd = pfds[i].fd;
    pfds.erase(pfds.begin() + i);
    Client* client = getClientById(fd);
    if (client)
    {
        std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :connection closed\r\n";

        for(size_t c = 0; c < _channels.size(); c++)
        {
            if (_channels[c].isMember(client))
            {
                const std::vector<Client*> &members = _channels[c].getMembers();
                for (size_t j = 0; j < members.size(); j++)
                {
                    if (members[j]->getFd() != fd)
                        members[j]->sendMessage(quitMsg);
                }
            }
        }
        for (size_t c = 0; c < _channels.size(); c++)
        {
            _channels[c].removeClientEverywhere(client);
        }
    }
    for (size_t c = 0; c < _channels.size(); c++)
    {
        if (_channels[c].isEmpty())
        {
            _channels.erase(_channels.begin() + c);
            c--;
        }
    }
    bool deletedClient = false;
    for (size_t j = 0; j < _clients.size(); j++)
    {
        if (_clients[j]->getFd() == fd)
        {
            delete _clients[j];
            _clients.erase(_clients.begin() + j);
            deletedClient = true;
            break;
        }
    }
	std::cout << "Client is Disconnected!" << std::endl;
    if (!deletedClient)
        close(fd);
}