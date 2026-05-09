#ifndef CLIENRT_HPP
#define CLIENRT_HPP
#include <string>
#include <poll.h>
#include <unistd.h>
class Client {
    private:
        int _fd;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        bool _authenticated;
        bool _registred;
        std::string _buffer;
        std::string _outBuffer;
        bool _toDisconnect;

    public:
        Client();
        Client(int fd);
        ~Client();

        int getFd() const;
        int setFd(int fd);
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        bool getToDisconnect() const;
        bool isAuth();
        bool isReg();
        std::string& getBuffer();


        void setNickname(std::string nickname);
        void setUsername(std::string username);
        void setRealname(std::string realname);
        void setToDisconnect(bool value);
        void setAuthenticated(bool value);
        void setRegistered(bool value);
        void appendToBuffer(std::string data);
        void clearBuffer();

        void sendMessage(const std::string& msg);
        std::string &getOutBuffer();
        const std::string &getOutBuffer() const;
        bool hasPendingOutput() const;
        bool hasPendingMessages() const;
        short getClientEvents() const;

};




#endif