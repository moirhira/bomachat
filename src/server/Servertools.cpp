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
	client_pfd.events = POLLIN | POLLOUT;
	client_pfd.revents = 0;
	this->pfds.push_back(client_pfd);
	std::cout << "New Client Accepted" << std::endl;
	fcntl(client_fd, F_SETFL, O_NONBLOCK);
}


void Server::removeClient(int fd)
{
    close(fd);
    for (std::vector<Client*>::iterator it = _clients.begin(); it != _clients.end(); )
    {
        if ((*it)->getFd() == fd)
        {
            delete *it;
            it = _clients.erase(it);
        }
        else
            ++it;
    }
    for (std::vector<pollfd>::iterator it = pfds.begin(); it != pfds.end(); )
    {
        if (it->fd == fd)
            it = pfds.erase(it);
        else
            ++it;
    }
	std::cout << "Client Removed (EOF)!" << std::endl;
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
    close(fd);
    pfds.erase(pfds.begin() + i);
    Client* client = getClientById(fd);
    if (client)
    {
        std::string quitMsg = ":" + client->getNickname() + "!" + client->getUsername() + "@localhost QUIT :connection closed\r\n";

        for(size_t c = 0; c < _channels.size(); c++)
        {
            if (_channels[c].isMember(client))
            {
                std::vector<Client*> members = _channels[c].getMembers();
                for (size_t j = 0; j < members.size(); j++)
                {
                    if (members[j]->getFd() != fd)
                    {
                        members[j]->sendMessage(quitMsg);
                    }
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

    for (size_t j = 0; j < _clients.size(); j++)
    {
        if (_clients[j]->getFd() == fd)
        {
            delete _clients[j];
            _clients.erase(_clients.begin() + j);
            break;
        }
    }
}