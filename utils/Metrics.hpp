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

        Metrics() : _clients(0), _channels(0), _messages(0), _connections(0) {
            _start = time(NULL);
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

        std::string serialize() const {
            time_t uptime = time(NULL) - _start;
            std::string out;
            out += "# HELP server_connected_clients Current connected clients\n";
            out += "# TYPE server_connected_clients gauge\n";
            out += "server_connected_clients " + to_str(_clients) + "\n\n";

            out += "# HELP server_channels_total Active channels\n";
            out += "# TYPE server_channels_total gauge\n";
            out += "server_channels_total " + to_str(_channels) + "\n\n";

            out += "# HELP server_messages_total Total messages sent\n";
            out += "# TYPE server_messages_total counter\n";
            out += "server_messages_total " + to_str(_messages) + "\n\n";

            out += "# HELP server_connections_total Total connection attemps\n";
            out += "# TYPE server_connections_total counter\n";
            out += "server_connections_total " + to_str(_connections) + "\n\n";

            out += "# HELP server_uptime_seconds Server uptime in seconds\n";
            out += "# TYPE server_uptime_seconds counter\n";
            out += "server_uptime_seconds " + to_str((int)uptime) + "\n";

            return out;
        }

};


#endif