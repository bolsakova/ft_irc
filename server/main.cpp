/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:01 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/04 00:33:36 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>
#include <fstream>  // Для логов
#include <sys/fcntl.h>

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./ircserv <port>\n";
        return 1;
    }

    // Открываем лог-файл
    std::ofstream log("server.log", std::ios::app);
    if (!log.is_open())
    {
        std::cerr << "Failed to open log file\n";
        return 1;
    }

    int listen_fd = create_and_bind(argv[1]);
    if (listen_fd < 0)
    {
        log << "Failed to create listening socket\n";
        return 1;
    }

    std::cout << "Server started on port " << argv[1] << " (minimal test)\n";
    log << "Server started on port " << argv[1] << " (minimal test)" << std::endl;

    sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    log << "Waiting for one client..." << std::endl;
    std::cout << "Waiting for one client..." << std::endl;

    // Цикл ожидания клиента
    int client_fd = -1;
    while (client_fd < 0)
    {
        client_fd = accept(listen_fd, (sockaddr *)&cli_addr, &cli_len);
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(100000); // 100 мс
                continue;
            }
            else
            {
                log << "Accept error: " << strerror(errno) << std::endl;
                perror("accept");
                close(listen_fd);
                return 1;
            }
        }
    }

    // Сделать client_fd блокирующим для минимального теста
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags & ~O_NONBLOCK);

    std::cout << "Client connected: "
              << inet_ntoa(cli_addr.sin_addr)
              << ":" << ntohs(cli_addr.sin_port) << "\n";
    log << "Client connected: "
        << inet_ntoa(cli_addr.sin_addr)
        << ":" << ntohs(cli_addr.sin_port) << std::endl;

    // Чтение данных
    char buffer[1024];
    ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0);

    if (n < 0)
    {
        log << "Recv error: " << strerror(errno) << std::endl;
        perror("recv");
    }
    else if (n == 0)
    {
        log << "Client disconnected" << std::endl;
        std::cout << "Client disconnected\n";
    }
    else
    {
        log << "Received: " << std::string(buffer, n) << std::endl;
        std::cout << "Received: " << std::string(buffer, n) << std::endl;

        // Отправка echo
        send(client_fd, buffer, n, 0);
        log << "Echo sent back" << std::endl;
        std::cout << "Echo sent back\n";
    }

    close(client_fd);
    close(listen_fd);

    log << "Server exit (minimal test)" << std::endl;
    log.close();

    std::cout << "Server exit (minimal test)\n";
    return 0;
}
