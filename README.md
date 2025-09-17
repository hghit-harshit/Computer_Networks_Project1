# Quiz Server-Client System

## Setup Steps
Ensure that you have the **GCC compiler** installed on your system. All the required source files are provided in the project directory. No additional files are needed to compile.  

## OS / Compiler Version
Tested with **GCC (g++)**. Works on Linux/MacOS/Windows environments where GCC is available.  

## Installation of Dependencies
Both client side and server side depend on **json.hpp**. Please keep this file in the same folder while compiling and running.  

## How to Run the Server and Clients

### Part 1(A)
To compile and run the server:
```
g++ server_A.cpp -o server_A  
./server_A  
```
To compile and run the client:
```
g++ client_A.cpp -o client_A  
./client_A  
```
### Part 1(B)
To compile and run the server:
```
g++ server_B.cpp -o server_B  
./server_B  
```
To compile and run the client:
```
g++ client_B.cpp -o client_B  
./client_B  
```
 
## Expected Input/Output Format

### Client Side
When you run the client, the program will prompt you to enter your **name**. After entering your name, it will ask you to choose your **preferred genre**. You must enter a number (1, 2, 3, or 4) corresponding to the genre options.  

### Example Input/Output

**Genre Selection**
```
Please select your prefered genre for quiz  
=========================================  
1. Biology  
2. Physics  
3. Chemistry  
4. Mathematics  
=========================================  
Enter your choice: 1  
Waiting for server to send the quiz...  
```
**Question Format**
```
Which organ is responsible for filtering waste from the blood in humans?  
A) Liver  
B) Kidneys  
C) Heart  
D) Lungs  
Enter your choice (A, B, C, D): A  
Wrong answer  
Correct answer is : "B"  
```
**Leaderboard Output**
```
--- LEADERBOARD ---  
Harshit: 0
```

