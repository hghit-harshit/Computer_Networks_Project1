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
    #include <netinet/tcp.h>
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
#include <thread>
#include <mutex>
#include <chrono>
#include <queue>
#include <fstream> // to read input from a file incase your llm fails

std::mutex leaderboard_mutex;
std::map<std::string,int> leaderboard;
std::mutex client_mutex;

bool something_failed = false;
// struct ClienInfo
// {
//     int socket;
//     std::string name;
// }
// std::map<std::string>

void print_error()
{
    #ifdef _WIN32
        std::cerr << WSAGetLastError() << std::endl;
    #else
    std::cerr << strerror(errno) << std::endl;
    #endif
}

int connect_to_llm()
{
    struct addrinfo hints;
    struct addrinfo *llm_addr;
    int status = 0;
    int socket_llm;
    //int new_fd;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET; // Ipv4
    hints.ai_socktype = SOCK_STREAM;// TCP
    hints.ai_flags = AI_PASSIVE; // fill my ip address for me

    if((status = getaddrinfo("192.168.50.142","25000",&hints,&llm_addr)) != 0)
    {
        std::cerr << "LLM Socket\n" << "gai error :" << gai_strerror(status);
    }

    socket_llm = socket(llm_addr->ai_family,llm_addr->ai_socktype,0);

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

    return socket_llm;
}

int reserve_llm(int socket_llmm)
{
    int socket_llm = connect_to_llm();
    std::string json = R"({"rollno":"es23btech11017"})";
    std::string request = "POST /reserve HTTP/1.1\r\n";
    request += "Host: 192.168.50.142:25000\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + std::to_string(json.size()) + "\r\n";
    //request += "Connection: close\r\n";
    request += "\r\n";
    request += json;

    send(socket_llm,request.c_str(),request.size(),0);
    char response[4028];
    int bytes = recv(socket_llm,response,sizeof(response),0);
    if(bytes < 0)
    {
        std::cout << "Bohot bheed h\n";
        something_failed = true;
    }
    std::cout << response;
    close(socket_llm);
   
    return 0;
}

/**
 * @brief 
 * Extract the json body from the response and returns it
 * @param http_response 
 * @return std::string 
 */
std::string extract_json_payload(const std::string &http_response)
{
    // Find the empty line separating headers and body
    std::size_t pos = http_response.find("\r\n\r\n");
    if (pos == std::string::npos) {
        throw std::runtime_error("No HTTP header/body separator found");
    }
    return http_response.substr(pos + 4); // skip "\r\n\r\n"
}

/**
 * @brief Get the questions object
 * 
 * @param http_body 
 * @return std::string 
 */
inline std::string get_questions(const std::string& http_body) 
{
    std::cout << "it gets here\n";
    nlohmann::json outer = nlohmann::json::parse(extract_json_payload(http_body));
    std::cout << "but not here\n";
    std::string inner_str = outer["choices"][0]["message"]["content"];
    std::cout << inner_str << std::endl;

    return inner_str;
}

/**
 * @brief 
 * Get question object and extract question and options from 
 * it to send to client and extract answers
 * @param questions 
 * @return std::pair<std::string,std::string> 
 */
std::pair<std::string,std::string> parse_questions(const std::string& questions)
{
    
    nlohmann::json quiz = nlohmann::json::parse(questions);

    nlohmann::json filtered_quiz; // the quiz that will be sent to client only contain questions and options
    filtered_quiz["message-type"] = "questions";
    std::string answers = ""; // well store answers as a string of chracters

    for (auto &question : quiz["questions"])
    {
        filtered_quiz["questions"].push_back({
            {"q",question["q"]},
            {"options",question["options"]}
        });
        std::cout << question["q"] << "\n";
        for (auto &opt : question["options"]) {
            std::cout << "  " << opt << "\n";
        }
        
        std::cout << "Answer: " << question["a"] << "\n\n";
        answers += question["a"];
    }

    return {filtered_quiz.dump(),answers};
}

std::pair<std::string,std::string> saved_questions(std::string& genre)
{
    std::fstream file("science_questions.txt");
    if(!file.is_open())
    {
        std::cerr << "Cant open the file and server is down as well hard luck buddy :(\n";
        return {"f","f"};
    }

    std::string file_data;
    std::string line;
    while(getline(file,line))
    {
        file_data += line;
    }
    std::cout << file_data << std::endl;
    return parse_questions(file_data);
    
}

std::pair<std::string,std::string> generate_quiz(std::string& genre,int socket_lllm)
{
    int socket_llm = connect_to_llm();
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

  
    std::string request = "POST /generate_quiz HTTP/1.1\r\n";
    request += "Host: 192.168.50.142:25000\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Content-Length: " + std::to_string(json.size()) + "\r\n";
    request += "\r\n";
    request += json;

    if(send(socket_llm,request.c_str(),request.size(),0) != request.size())
    {
        perror("send");
        std::cerr << "Trouble sending request to llm\n";
        something_failed = true;
    }
    char l_buffer[10000];
    std::string response = "";
    if(!something_failed && recv(socket_llm,l_buffer,sizeof(l_buffer),0) > 0)
    {
        response += (std::string)l_buffer + "\n";
    }
    else
    {
        //print_error();
        std::cout << "LLM crashed sending saved questions...\n";
        return saved_questions(genre);
    }
    close(socket_llm);
    std::cout << "response\n";
    std::cout << response;
    try
    {
        return parse_questions(get_questions(response));
    }
    catch(...)
    {
        std::cerr << "Failed to parse LLM's response\n";
        std::cout << "Sending saved questions....\n";
        return saved_questions(genre);
    }
    
}

void enable_keepalive(int socket) 
{
    int yes = 1;
    if (setsockopt(socket, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) < 0) {
        perror("setsockopt(SO_KEEPALIVE) failed");
    }

    // Optional: fine-tune parameters (Linux only)
    int idle = 10;     // seconds before sending keepalive probes
    int interval = 5;  // seconds between keepalive probes
    int count = 3;     // number of failed probes before declaring dead

    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle)) < 0)
        perror("setsockopt(TCP_KEEPIDLE) failed");
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval)) < 0)
        perror("setsockopt(TCP_KEEPINTVL) failed");
    if (setsockopt(socket, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count)) < 0)
        perror("setsockopt(TCP_KEEPCNT) failed");
}

std::string get_leaderboard()
{
    nlohmann::json json;
    for(auto& [name,score] : leaderboard)
    {
        json["standings"].push_back({
            {"name",name},
            {"score",score}
        });
    }
    json["message-type"] = "leaderboard";

    return json.dump();
}

void keep_alive_sender(int client_sock) // unused delete
{
    while (true) 
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::string ping = "{ \"message-type\" : \"KEEP_ALIVE\"}";
        if (send(client_sock, ping.c_str(), ping.size(), 0) <= 0) {
            std::cout << "[KeepAlive] Client probably disconnected.\n";
            break;
        }
        std::cout << "[KeepAlive] Sent PING\n";
    }
}

void handle_client(int client_socket,int socket_llm)
{
    
    char client_msg[1024];
    recv(client_socket,client_msg,sizeof(client_msg),0);

    nlohmann::json client_msg_json = nlohmann::json::parse((std::string)client_msg);

    std::cout << "client_msg:\n" << client_msg << std::endl;
    std::string client_name = client_msg_json["name"];
    std::string genre = client_msg_json["genre"];
    std::cout << "name: " << client_name << std::endl;
    std::cout << "genre: " << genre << std::endl;
    {
        std::lock_guard<std::mutex> lock(leaderboard_mutex);
        leaderboard[client_name] = 0;
    }
    reserve_llm(0);
    auto [quiz,answers] = generate_quiz(genre,socket_llm);

    const char* buffer = quiz.c_str();
    size_t total_data = quiz.size();
    size_t data_sent = 0;
    std::cout <<"THE QUIZ\n" <<quiz;
    while(data_sent < total_data)
    {
        int n = send(client_socket,buffer+data_sent,total_data - data_sent,0);
        if(n == -1)
        {
            perror("send");
            break;
        }
        data_sent += n;
    }

    char client_resp[1024];
    std::string server_resp;
    int question_no = 0;
    int bytes_received;
    auto last_keepalive = std::chrono::steady_clock::now();
    while(question_no < 10)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(client_socket, &readfds);

        // timeout = 1 second (so we wake up frequently)
        timeval tv{};
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int activity = select(client_socket + 1, &readfds, nullptr, nullptr, &tv);

        if (activity < 0) 
        {
            perror("select");
            break;
        }

        if(FD_ISSET(client_socket, &readfds))
        {
            if((bytes_received = recv(client_socket,client_resp,sizeof(client_resp),0)) <= 0)
            {
                std::cerr << "Client " << client_name << " disconnected in the middle of the quiz.\n";
                close(client_socket);
                return;
            }
            std::string msg(client_resp,bytes_received);
            std::cout << client_resp << std::endl;
            if(msg == "OK")
            {
                std::cout << "Received OK from client:" << client_name << std::endl;
                continue;
            }
            else if(msg.front() == 'L')
            {
                server_resp = get_leaderboard(); 
            }
            else
            {  
                if(answers[question_no] == msg.front())
                {
                    server_resp = "{\"message-type\" : \"Correct answer\"}";
                    {
                        std::lock_guard<std::mutex> lock(leaderboard_mutex);
                        leaderboard[client_name]++;
                    }
                }
                else
                {
                    nlohmann::json resp_json;
                    resp_json["message-type"] = "Wrong answer";
                    resp_json["answer"] = std::string(1, answers[question_no]);
                    server_resp = resp_json.dump();

                }
                question_no++;
            }
            send(client_socket,server_resp.c_str(),server_resp.size(),0);
        }

        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::seconds>(now - last_keepalive).count() >= 5) 
        {
            const char* keepalive_msg = "KEEP_ALIVE";
            send(client_socket, keepalive_msg, strlen(keepalive_msg), 0);
            std::cout << "Sent KEEP_ALIVE\n";
            last_keepalive = now;
        }
    }
    std::cout << client_name <<" has completed the quiz.\n";
    server_resp = get_leaderboard();
    send(client_socket,server_resp.c_str(),server_resp.size(),0);
    close(client_socket);
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
    
    struct addrinfo *llm_addr;
    
    //int new_fd;

    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET; // Ipv4
    hints.ai_socktype = SOCK_STREAM;// TCP
    hints.ai_flags = AI_PASSIVE; // fill my ip address for me

    int status = 0;
    if((status = getaddrinfo(NULL,"13017",&hints,&res)) != 0)
    {
        std::cout << "gai error :" << gai_strerror(status);
    }
    

    socketfd = socket(res->ai_family,res->ai_socktype,0);
    
    if(socketfd <0)
    {
        std::cerr << "Could not open socket\n";
        std::cerr << "Error : ";
        print_error();
        exit(1);
    }
    

    std::cout << "Resering LLM server instance..\n";
    //reserve_llm(socket_llm);
    std::cout << "LLM Reserverd\n";

    if(bind(socketfd,res->ai_addr,res->ai_addrlen) == -1)
    {
        std::cerr << "Bind failed\n";
        print_error();
        exit(1);
    }

    std::cout << "Listening for clients...\n";
    listen(socketfd,10);
    //std::cout << "got an request\n";
    while(true)
    {
        int new_client;
        struct sockaddr_storage client_addr;
        socklen_t addr_size = sizeof(client_addr);
        if(( new_client = accept(socketfd,(struct sockaddr*) &client_addr,&addr_size)) < 0)
        {
            std::cout << "Trouble accepting\n";
            print_error();
            exit(1);
        }
        else{ std::cout << "New client connected.\n";}
        enable_keepalive(new_client);
        std::thread(handle_client,new_client,socket_llm).detach();
    }
    

    #ifdef _WIN32
    closesocket(socket_llm);
    closesocket(socketfd);
    WSACleanup();
    #else
    close(socket_llm);
    close(socketfd);
    #endif
    
}
