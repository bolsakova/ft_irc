#include "inc/network/Server.hpp"
#include <iostream>
#include <cstdlib>

// Global server pointer for signal handler
Server* g_server = NULL;

void signalHandler(int signum) 
{
    if (g_server) 
	{
        std::cout << "\nShutting down server...\n";
        g_server->stop();
    }
}

int main(int ac, char* av[]) 
{
    if (ac != 3) 
	{
        std::cerr << "Usage: " << av[0] << " <port> <password>\n";
        return 1;
    }

    // Validate port
    int port = std::atoi(av[1]);
    if (port < 1024 || port > 65535) 
	{
        std::cerr << "Error: Port must be between 1024 and 65535\n";
        return 1;
    }

    std::string password = av[2];
    if (password.empty()) 
	{
        std::cerr << "Error: Password cannot be empty\n";
        return 1;
    }

    try 
	{
        Server server(port, password);
        g_server = &server;

        // Setup signal handlers
        signal(SIGINT, signalHandler);
        signal(SIGTERM, signalHandler);

        std::cout << "IRC Server starting on port " << port << "\n";
        server.run();  // Infinite loop with poll()
    } 
	catch (const std::exception& e) 
	{
        std::cerr << "Server error: " << e.what() << "\n";
        return 1;
    }
    return 0;
}


