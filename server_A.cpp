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
    #include <netdb.h>
    #include <errno.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <cstring>
#endif

#include "json.hpp"
#include <iostream>
#include <string>
#include <sstream>

void print_error()
{
    #ifdef _WIN32
        std::cerr << WSAGetLastError() << std::endl;
    #else
    std::cerr << strerror(errno) << std::endl;
    #endif
}

int reserve_llm(int socket_llm)
{
    std::string json = R"({"rollno":"es23btech11017"})";
    std::string request = "POST /reserve HTTP/1.1\r\n";
    request += "Host: 192.168.50.142:25000\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + std::to_string(json.size()) + "\r\n";
    request += "\r\n";
    request += json;

    send(socket_llm,request.c_str(),request.size(),0);
    char response[4028];
    int bytes = recv(socket_llm,response,sizeof(response),0);
    //response[bytes] = '\0';
    //std::cout << response  << std::endl;
    //std::cout << std::flush;
    return 0;
}
std::string extract_json_payload(const std::string &http_response)
{
    // Find the empty line separating headers and body
    std::size_t pos = http_response.find("\r\n\r\n");
    if (pos == std::string::npos) {
        throw std::runtime_error("No HTTP header/body separator found");
    }
    return http_response.substr(pos + 4); // skip "\r\n\r\n"
}

std::string parse_response(const std::string& response)
{
    std::cout << "it gets here\n";
    nlohmann::json outer = nlohmann::json::parse(extract_json_payload(response));
    std::cout << "but not here\n";
    std::string inner_str = outer["choices"][0]["message"]["content"];
    std::cout << inner_str << std::endl;
    nlohmann::json quiz = nlohmann::json::parse(inner_str);
    for (auto &q : quiz["questions"])
    {
        std::cout << q["q"] << "\n";
        for (auto &opt : q["options"]) {
            std::cout << "  " << opt << "\n";
        }
        std::cout << "Answer: " << q["a"] << "\n\n";
    }

    return inner_str;
}

std::string generate_quiz(std::string genre,int socket_llm)
{
    std::string json = R"({
    "rollno": "es23btech11017",
    "messages": [
        {
            "role": "system",
            "content": "You are a trivia generator. Given a genre, output exactly 10 multiple-choice questions. Each question must have 4 options (A, B, C, D) and one correct answer. Return ONLY valid JSON in the following format: {\"questions\":[{\"q\":\"...\",\"options\":[\"A) ...\",\"B) ...\",\"C) ...\",\"D) ...\"],\"a\":\"A\"}, ...]}"
        },
        {
            "role": "user",
            "content": "You are a trivia generator. Given a genre, output exactly 10 multiple-choice questions. Each question must have 4 options (A, B, C, D) and one correct answer. Return ONLY valid JSON in the following format: {\"questions\":[{\"q\":\"...\",\"options\":[\"A) ...\",\"B) ...\",\"C) ...\",\"D) ...\"],\"a\":\"A\" Generate 10 MCQ trivia questions about )"+ genre + R"(."
        }
    ],
    "temperature": 0.0,
    "max_tokens": 1200
})";


    std::cout << "is this even getting called??\n";
    std::string request = "POST /generate_quiz HTTP/1.1\r\n";
    request += "Host: 192.168.50.142:25000\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + std::to_string(json.size()) + "\r\n";
    request += "\r\n";
    request += json;

    // std::cout << "===== REQUEST =====\n";
    // std::cout << request << "\n";
    // std::cout << "===== END REQUEST =====\n";


    while(send(socket_llm,request.c_str(),request.size(),0) != request.size())
    {
        std::cout << "count\n";
    }
    // {
    //     std::cerr << "Trouble sending request to llm\n";
    //     print_error();
    // }
    char l_buffer[10000];
    std::string wresponse = "";
    if(recv(socket_llm,l_buffer,sizeof(l_buffer),0) > 0)
    {
        std::cout << "getting here\n";
        wresponse += (std::string)l_buffer + "\n";
    }
    else
    {
        std::cout << "are we getting an erro\n";
        print_error();
    }
    std::cout << "response\n";
    //std::cout << wresponse;
    return parse_response(wresponse);

}

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
    int socket_llm;
    struct addrinfo hints,*res;
    struct sockaddr_storage client_addr;
    struct addrinfo *llm_addr;
    socklen_t addr_size = sizeof(client_addr);
    int new_fd;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET; // Ipv4
    hints.ai_socktype = SOCK_STREAM;// TCP
    hints.ai_flags = AI_PASSIVE; // fill my ip address for me

    int status = 0;
    if((status = getaddrinfo(NULL,"13017",&hints,&res)) != 0)
    {
        std::cout << "gai error :" << gai_strerror(status);
    }
    if((status = getaddrinfo("192.168.50.142","25000",&hints,&llm_addr)) != 0)
    {
        std::cerr << "LLM Socket\n" << "gai error :" << gai_strerror(status);
    }

    socketfd = socket(res->ai_family,res->ai_socktype,0);
    socket_llm = socket(llm_addr->ai_family,llm_addr->ai_socktype,0);
    if(socketfd <0)
    {
        std::cerr << "Could not open socket\n";
        std::cerr << "Error : ";
        print_error();
        exit(1);
    }
    if(socket_llm <0)
    {
        std::cerr << "Could not open llm socket\n";
        std::cerr << "Error : ";
        print_error();
        exit(1);
    }

    if(connect(socket_llm,llm_addr->ai_addr,llm_addr->ai_addrlen) != 0)
    {
        std::cerr << "Could not connect to llm server.\n";
        print_error();
        exit(1);
    }
    else{ std::cout << "Connect to LLM Server\n";}

    std::cout << "Resering LLM server instance\n";
    reserve_llm(socket_llm);

    if(bind(socketfd,res->ai_addr,res->ai_addrlen) == -1)
    {
        std::cerr << "Bind failed\n";
        print_error();
        exit(1);
    }

    std::cout << "Listening for client...\n";
    listen(socketfd,10);
    //std::cout << "got an request\n";
    if(( new_fd = accept(socketfd,(struct sockaddr*) &client_addr,&addr_size)) < 0)
    {
        std::cout << "Trouble accepting\n";
        print_error();
        exit(1);
    }

    char response[1024];
    recv(new_fd,response,sizeof(response),0);
    std::cout << response;
    char responce[] = "Hello client\n";
    std::string genre = std::string(response);
    std::string quiz = generate_quiz("science",socket_llm);
    const char* buffer = quiz.c_str();
    size_t total_data = quiz.size();
    size_t data_sent = 0;
    std::cout <<"THE QUIZ\n" <<quiz;
    while(data_sent < total_data)
    {
        int n = send(new_fd,buffer+data_sent,total_data - data_sent,0);
        if(n == -1)
        {
            perror("send");
            break;
        }

        data_sent += n;

    }
    std::cout << "print this atlease plase\n";
    //getchar();

    #ifdef _WIN32
    closesocket(socket_llm);
    closesocket(new_fd);
    closesocket(socketfd);
    WSACleanup();
    #else
    close(socket_llm);
    close(new_fd);
    close(socketfd);
    #endif

}