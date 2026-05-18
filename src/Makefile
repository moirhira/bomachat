NAME = bomachat
SRC  = main.cpp \
	server/Server.cpp \
	server/Servertools.cpp \
	server/Client.cpp \
	server/Channel.cpp \
	utils/Parser.cpp \
	commands/ServerCommands.cpp \
	commands/Invite.cpp \
	commands/Join.cpp \
	commands/Kick.cpp \
	commands/Mode.cpp \
	commands/Nick.cpp \
	commands/Part.cpp \
	commands/Pass.cpp \
	commands/Privmsg.cpp \
	commands/Quit.cpp \
	commands/Topic.cpp \
	commands/User.cpp

OBJ = $(SRC:.cpp=.o)
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iserver -Icommands -Iutils -lpthread


all: $(NAME)


$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME) 

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bot:
	make -C bot

clean:
	make -C bot clean
	rm -f $(OBJ)

fclean: clean
	make -C bot fclean
	rm -f $(NAME)

re: fclean all bot

.PHONY: all clean fclean re bot