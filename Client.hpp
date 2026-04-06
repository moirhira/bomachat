#ifndef CLIENRT_HPP
#define CLIENRT_HPP
#include <string>


class Client {
    private:
        int _fd;
        std::string _nickname;
        std::string _username;
        std::string _realname;
        bool _authenticated;
        bool _registred;
        std::string _buffer;

    public:
        Client(int fd);
        ~Client();

        int getFd const();
        std::string getNickname() const;
        std::string getUsername() const;
        std::string getRealname() const;
        bool isAuth();
        bool isReg();
        std::string getBuffer() const;


        void setNickname(std::string nickname);
        void setUsername(std::string username);
        void setRealname(std::string realname);
        void setAuthenticated(bool value);
        void setRegistered(bool value);
        void appendToBuffer(std::string data);
        void clearBuffer();

};




#endif