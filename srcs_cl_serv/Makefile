# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2025/12/03 23:58:28 by aokhapki          #+#    #+#              #
#    Updated: 2025/12/07 00:55:27 by aokhapki         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #


# Makefile для тестирования IRC сервера
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror

TARGET = ircserv

SRCS = main.cpp server/Server.cpp client/Client.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: all clean fclean re


