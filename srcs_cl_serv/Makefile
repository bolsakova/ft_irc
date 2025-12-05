# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/03 23:58:28 by aokhapki          #+#    #+#              #
#    Updated: 2025/12/04 01:56:42 by aokhapki         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


# Makefile для тестирования IRC сервера

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror
TARGET = ircserv
OBJS = server.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

server.o: server.cpp server.hpp
	$(CXX) $(CXXFLAGS) -c server.cpp -o server.o

main.o: main.cpp server.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re

