#include <bits/stdc++.h>
#include <iostream>
#include <thread>
#include <vector>
#include <string>

#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <cstring>

#define PORT 5000        // Port to listen on
#define BUFFER_SIZE 1024 // Buffer size for receiving data

using namespace std;

// Mutex to ensure thread-safe console output
// mutex consoleMutex;

// Function to handle a single client connection
void handleConnection(int clientSocket, sockaddr_in clientAddress)
{
    char buffer[BUFFER_SIZE];
    string clientIP = inet_ntoa(clientAddress.sin_addr);
    int clientPort = ntohs(clientAddress.sin_port);

    {
        /// lock_guard<mutex> lock(consoleMutex);
        cout << "Connection accepted from " << clientIP << ":" << clientPort << endl;
    }

    // Read data from the client
    ssize_t bytesRead;
    while ((bytesRead = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0)) > 0)
    {
        buffer[bytesRead] = '\0'; // Null-terminate the received data
        {
            // lock_guard<mutex> lock(consoleMutex);
            cout << "Received frame: " << buffer << endl;
        }
    }

    if (bytesRead == 0)
    {
        {
            // lock_guard<mutex> lock(consoleMutex);
            cout << "Connection closed by client " << clientIP << ":" << clientPort << endl;
        }
    }
    else if (bytesRead < 0)
    {
        // lock_guard<mutex> lock(consoleMutex);
        cerr << "Error reading from client " << clientIP << ":" << clientPort << endl;
    }

    close(clientSocket); // Close the client socket
}

int main()
{
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddress{}, clientAddr{};
    int addrLen = sizeof(clientAddr);
    char buffer[1024];

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        cerr << "Failed to initialize Winsock.\n";
        return -1;
    }
    // Create the server socket
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        cerr << "Failed to create server socket." << endl;
        return EXIT_FAILURE;
    }

    // Configure the server address structure
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // Bind the server socket
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "Failed to bind server socket." << endl;
        close(serverSocket);
        WSACleanup();
        return EXIT_FAILURE;
    }

    // Start listening for incoming connections
    if (listen(serverSocket, 10) == -1)
    {
        cerr << "Failed to listen on server socket." << endl;
        close(serverSocket);
        return EXIT_FAILURE;
    }

    cout << "Receiver started. Waiting for connections on port " << PORT << "..." << endl;

    // Vector to keep track of threads for handling connections
    vector<thread> connectionThreads;
    int n=0;
    while (n<2)
    {
        sockaddr_in clientAddress{};
        int clientAddressLen = sizeof(clientAddress);

        // Accept a new connection
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLen);
        if (clientSocket == -1)
        {
            cerr << "Failed to accept connection." << endl;
            continue;
        }
        n++;
        // Handle the client in a separate thread
        connectionThreads.emplace_back(handleConnection, clientSocket, clientAddress);
    }

    // Wait for all threads to complete
    for (auto &t : connectionThreads)
    {
        if (t.joinable())
        {
            t.join();
        }
    }
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
