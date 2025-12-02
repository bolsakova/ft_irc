#include <arpa/inet.h>

// or #include <netinet/in.h>


int main()
{
    int server_fd;
    int client_fd;

    sockaddr_in servaer_addr;
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    return 0;
}