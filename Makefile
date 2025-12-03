# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/03 23:58:28 by aokhapki          #+#    #+#              #
#    Updated: 2025/12/04 00:10:05 by aokhapki         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


# Makefile для тестирования IRC сервера

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror
TARGET = ircserv
OBJS = server/server.o server/main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

server/server.o: server/server.cpp server/server.hpp
	$(CXX) $(CXXFLAGS) -c server/server.cpp -o server/server.o

server/main.o: server/main.cpp server/server.hpp
	$(CXX) $(CXXFLAGS) -c server/main.cpp -o server/main.o

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re

