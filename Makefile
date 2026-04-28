NAME = ircserv
BOT  = bot
SRC  = main.cpp \
	server/Server.cpp \
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

BOT_SRC = bot/Bot.cpp \
	bot/CommandHandler.cpp
BOT_OBJ = $(BOT_SRC:.cpp=.o)
OBJ = $(SRC:.cpp=.o) $(BOT_OBJ)
CXX = c++
CXXFLAGS = -Wall -Wextra -Werror -std=c++98 -Iserver -Icommands -Iutils


all: $(NAME)


$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

$(BOT): $(BOT_OBJ)
	$(CXX) $(CXXFLAGS) $(BOT_OBJ) -o $(BOT)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re