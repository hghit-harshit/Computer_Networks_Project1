## How to Run the Server and Clients
Both client side and server side depend on "json.hpp"
please keep that in the same folder while running.
### Part 1(A)
- To compile and run the server:
  ```bash
  g++ server_A.cpp -o server_A
  ./server_A
- To compile and run the client:
g++ client_A.cpp -o client_A
./client_A

### Part 1(B)
- To compile and run the server:
  ```bash
  g++ server_B.cpp -o server_B
  ./server_B
- To compile and run the client:
g++ client_B.cpp -o client_B
./client_B


## Expected Input/Output Format

### Client Side
- When you run the client, the program will prompt you to enter your **name**.  
- After entering your name, it will ask you to choose your **preferred genre**.  
- You must enter a number (`1`, `2`, `3`, or `4`) corresponding to the genre options.  


###########Example Input Output#########

Please select your prefered genre for quiz
=========================================
1.Biology
2.Physics
3.Chemistry
4.Mathematics
=========================================
Enter your choice:1
Waiting for server to send the quiz...

#######How Question Looks######
Which organ is responsible for filtering waste from the blood in humans?
A) Liver
B) Kidneys
C) Heart
D) Lungs
Enter your choice(A,B,C,D):A
Wrong answer
Correct answer is : "B"
################################
### How leader board looks#####
--- LEADERBOARD ---
Harshit:0
#####################