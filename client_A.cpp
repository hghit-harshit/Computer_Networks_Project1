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
#include "json.hpp"
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
//void 


bool running = true;
std::mutex queue_mutex;
std::queue<std::string> message_queue;
std::condition_variable cv;

void display_menu()
{
    std::cout << "Enter Your choice\n";
    std::cout << "========================\n";
    std::cout << "1.Next Question\n";
    std::cout << "2.Leaderboard\n";
    std::cout << "========================\n";
    std::cout << "Your choice : ";
}

std::string select_genre()
{
    int choice;
    std::vector<std::string>genres = {"Biology","Physics","Chemistry","Mathematics"};
    std::cout << "Please selce your prefered genre for quiz\n";
    std::cout << "=========================================\n";
    std::cout << "1.Biology\n";
    std::cout << "2.Physics\n";
    std::cout << "3.Chemistry\n";
    std::cout << "4.Mathematics\n";
    std::cout << "=========================================\n";
    std::cout << "Enter your choice:";
    std::cin >> choice;
    while( choice < 0 && choice > genres.size())
    {
        std::cout << "Please enter valid choice\n";
        std::cout << "Enter your choice:";
        std::cin >> choice;
    }

    return genres[choice-1];
}


void receiver_thread(int socketfd) {
    char buffer[10000];
    while (running) 
    {
        int bytes_received = recv(socketfd, buffer, sizeof(buffer)-1, 0);
        if (bytes_received <= 0) 
        {
            std::cerr << "Server disconnected or error.\n";
            running = false;
            break;
        }
        buffer[bytes_received] = '\0';
        std::string msg(buffer);

        try 
        {
            //auto json_msg = nlohmann::json::parse(msg);

            if (msg == "KEEP_ALIVE") 
            {
                send(socketfd, "OK", 2, 0);
            } else 
            {
                std::lock_guard<std::mutex> lock(queue_mutex);
                message_queue.push(msg);
                cv.notify_one();
            }
        } catch (...) {
            std::cerr << "Invalid JSON received: " << msg << "\n";
        }
    }
}

void print_leaderboard(const std::string& raw_json)
{
    std::cout << raw_json << std::endl;
    nlohmann::json leaderboard_json = nlohmann::json::parse(raw_json);
    std::cout << "\n--- LEADERBOARD ---\n";
    for(auto standing : leaderboard_json["standings"])
    {
        std::cout << standing["name"] << ":" << standing["score"] << std::endl;
    }

}

void handle_leaderboard(int socketfd,const std::string answer) // unused
{
    if(send(socketfd,answer.c_str(),answer.size(),0) == -1)
    {
        std::cerr << "Could not send request for leaderboard\n";
    }
    char leaderboard_buffer[4096];
    while(true)
    {
        int bytes_received = recv(socketfd,leaderboard_buffer,sizeof(leaderboard_buffer)-1,0);
        std::string msg(leaderboard_buffer);
        auto msg_json = nlohmann::json::parse(msg);
        if (bytes_received > 0) 
        {
            if(msg_json["message-type"] == "Leaderboard")
            {
                print_leaderboard(std::string(leaderboard_buffer));
                break;
            }
            else if(msg_json["message-type"] == "KEEP_ALIVE")
                send(socketfd,"OK",2,0);
        } 
        else 
        {
            std::cerr << "Could not retrieve leaderboard data.\n";
        }
    }
}

inline void get_leaderboard(int socketfd)
{
    send(socketfd, "L", 1, 0);

    std::unique_lock<std::mutex> lb_lock(queue_mutex);
    cv.wait(lb_lock, []{ return !message_queue.empty(); });
    std::string lb_msg = message_queue.front();
    message_queue.pop();
    lb_lock.unlock();

    print_leaderboard(lb_msg);
}

void print_result(const std::string& resp_msg)
{

    auto json = nlohmann::json::parse(resp_msg);
    if(json["message-type"] == "Correct answer")
    {
        std::cout << "Correct Answer!\n";
    }
    else
    {
        std::cout << "Wrong answer\n";
        std::cout << "Correct answer is : " << json["answer"] << std::endl;
    }
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
    struct addrinfo hints,*server_add;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    //hints.ai_flags = AI_PASSIVE; // fill my ip address for me

    int status = 0;
    if((status = getaddrinfo("192.168.50.111","13017",&hints,&server_add)) != 0)
    {
        std::cout << "gai error :" << gai_strerror(status);
    }

    socketfd = socket(server_add->ai_family,server_add->ai_socktype,0);
    if(socketfd <0)
    {
        std::cerr << "Could not open socket\n";
        exit(1);
    }

    if(connect(socketfd,server_add->ai_addr,server_add->ai_addrlen) != 0)
    {
        int err = WSAGetLastError();
        std::cerr << "Connect failed. Error: " << err << "\n";
    }
    
    std::thread(receiver_thread,socketfd).detach();

    std::string name,genre;
    std::cout << "Please enter you name: ";
    std::cin >> name;
    genre = select_genre();

    //char msg[] = "Science";
    nlohmann::json msg_json;
    msg_json["name"] = name;
    msg_json["genre"] = genre;
    send(socketfd,msg_json.dump().c_str(),msg_json.dump().size(),0);
    std::cout << "data sent\n";
    std::cout << "Waiting for server to send the quiz...\n";
    
    
    std::unique_lock<std::mutex> lock(queue_mutex);
    cv.wait(lock, []{ return !message_queue.empty(); });
    std::string quiz_msg = message_queue.front();
    message_queue.pop();
    lock.unlock();
    
    nlohmann::json response_json = nlohmann::json::parse(quiz_msg);

    for(const  auto& question : response_json["questions"])
    {
        std::cout << question["q"] << std::endl;
        for(const auto& option : question["options"])
        {
            std::cout << option << std::endl;
        }

        std::cout << "Enter your choice(A,B,C,D):";
        std::string answer; 
        std::cin >> answer;
        char server_resp[1024];
        if(send(socketfd,answer.c_str(),answer.size(),0) == -1)
        {
            std::cerr << "Could not send answer\n";
        }
      
        std::unique_lock<std::mutex> resp_lock(queue_mutex);
        cv.wait(resp_lock, []{ return !message_queue.empty(); });
        std::string resp_msg = message_queue.front();
        message_queue.pop();
        resp_lock.unlock();
        std::cout << resp_msg << std::endl;
        print_result(resp_msg);

        while(true)
        {
            std::cout <<"=======================\n";
            std::cout << "What do you want to do\n";
            std::cout << "Next question(type N)\n";
            std::cout << "See Leaderboard(type L)\n";
            std::cout <<"=======================\n";
            std::cin >> answer;
            if(answer == "N")
            {
                break;
            }
            else if(answer == "L")
            {
                //handle_leaderboard(socketfd,answer);
                get_leaderboard(socketfd);
            }
            else
            {
                std::cout << "Invalid Entry!\n";
            }
        }
    }

    std::cout << "You have completed the Quiz!!\n";
    std::cout << "Your score is " << "score" << " out of 10\n";
    get_leaderboard(socketfd);
    
}