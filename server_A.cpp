#ifdef _WIN32
    // Windows (both 32-bit and 64-bit)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")   // For MSVC, links Winsock
#else
    // Linux / macOS (POSIX)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include <iostream>

int main(int argc, char* argv[])
{
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            printf("WSAStartup failed. Error Code: %d\n", WSAGetLastError());
            return 1;
        }
    #endif
    int socketfd;
    struct addrinfo hints,*res;
    struct sockaddr_storage their_addr;
    socklen_t addr_size = sizeof(their_addr);
    int new_fd;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // fill my ip address for me

    int status = 0;
    if((status = getaddrinfo(NULL,"3490",&hints,&res)) != 0)
    {
        std::cout << "gai error :" << gai_strerror(status);
    }
    std::cout << res->ai_addr->sa_data << '\n';

    socketfd = socket(res->ai_family,res->ai_socktype,0);
    if(socketfd <0)
    {
        std::cout << "Could not open socket\n";
        exit(1);
    }

    if(bind(socketfd,res->ai_addr,res->ai_addrlen) == -1)
    {
        std::cerr << "Bind failed\n";
    }
    std::cout << "listening\n";
    listen(socketfd,10);
    //std::cout << "got an request\n";
    if(( new_fd= accept(socketfd,(struct sockaddr*) &their_addr,&addr_size)) < 0)
    {
        std::cout << "Trouble accepting\n";
        exit(1);
    }

    char buffer[1024];
    recv(new_fd,buffer,strlen(buffer),0);
    std::string data_received(buffer);
    std::cout << data_received << '\n';
}