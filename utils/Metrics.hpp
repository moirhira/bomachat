#ifndef METRICS_HPP
#define METRICS_HPP
#include <string>
#include <ctime>

class Metrics {
    private:
        int _clients;
        int _channels;
        int _messages;
        int _connections;
        time_t _start;

        static std::string to_str(int n) {
            char buf[32];
            sprintf(buf, "%d", n);
            return std::string(buf);
        }

    public:
        static Metrics& instance() {
            static Metrics inst;
            return inst;
        }
        

        void incrementMessages() {_messages++; };
        void incrementConnections() {_connections++; };
        void setClient(int n) {_clients = n; };
        void setChannels(int n) {_channels = n; };



};


#endif