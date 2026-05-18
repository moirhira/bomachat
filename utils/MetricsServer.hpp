#ifndef METRICS_SERVER_HPP
#define METRICS_SERVER_HPP
#include "Metrics.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <pthread.h>


class MetricsServer {
    public:
        MetricsServer(int port) : _port(port), _fd(-1) {}

        void start() {
            pthread_t thread;
            pthread_create(&thread, NULL, &MetricsServer::run, this);
            pthread_detach(thread);
        }

    private:
        int _port;
        int _fd;

        static void* run(void* arg) {
            MetricsServer* self = static_cast<MetricsServer*>(arg);
            self->serve();
            return NULL;
        }

        void serve() {
            _fd = socket(AF_INET, SOCK_STREAM, 0);
            int opt = 1;
            setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

            struct sockaddr_in addr;
            memset(&addr, 0, sizeof(addr));
            addr.sin_family         =   AF_INET;
            addr.sin_addr.s_addr    =   INADDR_ANY;
            addr.sin_port           =   htons(_port);

            bind(_fd, (struct sockaddr*)&addr, sizeof(addr));
            listen(_fd, 10);

            while (true) {
                int client = accept(_fd, NULL, NULL);
                if (client < 0)
                    continue;
                
                std::string body = Metrics::instance().serialize();
                std::string response = 
                    "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/plain; version=0.0.4\r\n"
                    "Content-Length: " + to_str(body.size()) + "\r\n"
                    "\r\n" + body;

                send(client, response.c_str(), response.size(), 0);
                close(client);
            }   
        }

        static std::string to_str(size_t n) {
            char buf[32];
            sprintf(buf, "%zu", n);
            return std::string(buf);
        }
};

#endif