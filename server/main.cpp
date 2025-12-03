/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: aokhapki <aokhapki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/03 23:14:01 by aokhapki          #+#    #+#             */
/*   Updated: 2025/12/04 00:40:58 by aokhapki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "server.hpp"        // Для функций create_and_bind(), структуры Client (потом)
#include <sys/socket.h>      // socket, bind, listen, accept
#include <netinet/in.h>      // sockaddr_in, htons, ntohs
#include <arpa/inet.h>       // inet_ntoa
#include <unistd.h>          // close, read, write
#include <iostream>          // std::cout, std::cerr
#include <cstring>           // strerror, memset
#include <cerrno>            // errno для проверки ошибок
#include <fstream>           // std::ofstream для логов
#include <chrono>            // для текущего времени
#include <iomanip>           // std::put_time для форматирования времени
#include <sstream>           // std::ostringstream для формирования строки
#include <sys/fcntl.h>

// Функция для получения текущего времени в читаемом формате
std::string current_time()
{
    auto now = std::chrono::system_clock::now();       // получаем текущее время системы
    std::time_t t = std::chrono::system_clock::to_time_t(now); // переводим в time_t
    std::tm tm{};
    localtime_r(&t, &tm);                              // потокобезопасное преобразование в локальное время
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");   // форматируем строку "ГГГГ-ММ-ДД ЧЧ:ММ:СС"
    return oss.str();
}

// Макрос для упрощения логирования с текущим временем
#define LOG(msg) log << "[" << current_time() << "] " << msg << std::endl

int main(int argc, char **argv)
{
    // Проверяем аргументы командной строки
    // ./ircserv <port>
    if (argc < 2)
    {
        std::cerr << "Usage: ./ircserv <port>\n";
        return 1;
    }

    // Открываем файл для логирования событий сервера
    // std::ios::app означает, что все записи будут добавляться в конец файла
    std::ofstream log("server.log", std::ios::app);
    if (!log.is_open())
    {
        std::cerr << "Failed to open log file\n";
        return 1;
    }

    // Создаём слушающий TCP-сокет и привязываем его к указанному порту
    int listen_fd = create_and_bind(argv[1]);
    if (listen_fd < 0)
    {
        LOG("Failed to create listening socket"); // пишем в лог
        return 1;
    }

    // Информационное сообщение в терминал и лог
    std::cout << "Server started on port " << argv[1] << " (minimal test)\n";
    LOG("Server started on port " << argv[1] << " (minimal test)");

    // Структура для хранения адреса клиента
    sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    LOG("Waiting for one client...");
    std::cout << "Waiting for one client..." << std::endl;

    // Цикл ожидания подключения клиента
    // Используем неблокирующий слушающий сокет, поэтому accept() может вернуть -1 с errno=EAGAIN
    int client_fd = -1;
    while (client_fd < 0)
    {
        client_fd = accept(listen_fd, (sockaddr *)&cli_addr, &cli_len); // пытаемся принять подключение
        if (client_fd < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // Пока нет клиентов, ждём 100 мс и повторяем
                usleep(100000);
                continue;
            }
            else
            {
                // Любая другая ошибка — логируем и выходим
                LOG("Accept error: " << strerror(errno));
                perror("accept");
                close(listen_fd);
                return 1;
            }
        }
    }

    // После успешного accept() делаем клиентский сокет **блокирующим** для простого минимального теста
    // Чтобы recv() ждал данные от клиента, а не сразу возвращал EAGAIN
    int flags = fcntl(client_fd, F_GETFL, 0);        // получаем текущие флаги сокета
    fcntl(client_fd, F_SETFL, flags & ~O_NONBLOCK); // снимаем флаг неблокирующего режима

    // Информация о подключенном клиенте
    std::cout << "Client connected: "
              << inet_ntoa(cli_addr.sin_addr)        // IP адрес клиента
              << ":" << ntohs(cli_addr.sin_port)    // порт клиента
              << "\n";
    LOG("Client connected: "
        << inet_ntoa(cli_addr.sin_addr)
        << ":" << ntohs(cli_addr.sin_port));

    // Буфер для приёма данных
    char buffer[1024];
    ssize_t n = recv(client_fd, buffer, sizeof(buffer), 0); // читаем данные

    if (n < 0)
    {
        // Ошибка чтения
        LOG("Recv error: " << strerror(errno));
        perror("recv");
    }
    else if (n == 0)
    {
        // Клиент закрыл соединение
        LOG("Client disconnected");
        std::cout << "Client disconnected\n";
    }
    else
    {
        // Получили данные, пишем в лог и на экран
        LOG("Received: " << std::string(buffer, n));
        std::cout << "Received: " << std::string(buffer, n) << std::endl;

        // Отправка echo обратно клиенту
        send(client_fd, buffer, n, 0);
        LOG("Echo sent back");
        std::cout << "Echo sent back\n";
    }

    // Закрываем сокеты
    close(client_fd);
    close(listen_fd);

    LOG("Server exit (minimal test)");
    log.close();

    std::cout << "Server exit (minimal test)\n";
    return 0;
}

