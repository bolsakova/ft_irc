#include "network/Server.hpp"
#include <iostream>
#include <cstdlib>
#include <csignal>

// Global server pointer for signal handler
Server* g_server = NULL;

void signalHandler(int signum) 
{
    (void)signum;//added to avoid unused parameter warning
    if (g_server) 
	{
        std::cout << "\nShutting down server...\n";
        g_server->stop(); //Ctrl+C / kill
    }
}

int main(int ac, char* av[]) 
{
    if (ac != 3) 
	{
        std::cerr << "Usage: " << av[0] << " <port> <password>\n";
        return 1;
    }
    std::string port_str = av[1];
    std::string password = av[2];
    if (password.empty()) 
	{
        std::cerr << "Error: Password cannot be empty\n";
        return 1;
    }
    try 
	{
        Server server(port_str, password);
        g_server = &server;

        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        std::cout << "IRC Server starting on port " << port_str << "\n";
        server.run();  // Infinite loop with poll()
    } 
	catch (const std::exception& e) 
	{
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
