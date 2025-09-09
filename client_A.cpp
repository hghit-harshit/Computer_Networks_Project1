#ifdef _WIN32
    // Windows (both 32-bit and 64-bit)
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")   // For MSVC, links Winsock
#else
    // Linux / macOS (POSIX)
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
#endif

#include <iostream>

//void 
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
    struct addrinfo hints,*server_add;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET6;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE; // fill my ip address for me

    int status = 0;
    if((status = getaddrinfo("::1","3490",&hints,&server_add)) != 0)
    {
        std::cout << "gai error :" << gai_strerror(status);
    }

    socketfd = socket(server_add->ai_family,server_add->ai_socktype,0);
    if(socketfd <0)
    {
        std::cerr << "Could not open socket\n";
        exit(1);
    }

    // if(connect(socketfd,server_add->ai_addr,server_add->ai_addrlen) != 0)
    // {
    //     //std::cerr << "Could not connect to the host\n";
    //     std::cerr << "Connect failed. Error: " << WSAGetLastError() << "\n";
    //     //exit(1);
    // }
    std::cout << connect(socketfd,server_add->ai_addr,server_add->ai_addrlen);
    int err = WSAGetLastError();
    std::cerr << "Connect failed. Error: " << err << "\n";
    
    char msg[] = "Hello server\n";
    send(socketfd,msg,strlen(msg),0);
    std::cout << "data sent\n";
}