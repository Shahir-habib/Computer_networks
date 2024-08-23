#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
using namespace std;

#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib for Winsock functions
#pragma comment(lib, "iphlpapi.lib")

#include "crc.h"
#include "checksum.h"

#define BUFFER_SIZE 1000

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void clear_buffer(char *buffer, size_t size)
{
    memset(buffer, '\0', size);
}

int main(int argc, char *argv[])
{
    WSADATA wsaData;
    if (argc < 2)
    {
        fprintf(stderr, "Port number not provided. Program terminated.\n");
        return 1;
    }

    SOCKET socket_fd, new_socket_fd;
    int port_number;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // Create a socket
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_SOCKET)
    {
        printf("Error opening socket: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Set up the server address
    port_number = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_number);

    // Bind the socket
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        printf("Error on binding: %d\n", WSAGetLastError());
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(socket_fd, 5) == SOCKET_ERROR)
    {
        printf("Error on listen: %d\n", WSAGetLastError());
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    // Accept a connection
    new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
    if (new_socket_fd == INVALID_SOCKET)
    {
        printf("Error on accept: %d\n", WSAGetLastError());
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    printf("Server: Connection established\n");

    // Receive and convert size
    int size_network_order;
    ssize_t size_recv_total = 0;
    ssize_t size_left = sizeof(size_network_order);
    while (size_recv_total < size_left)
    {
        ssize_t received = recv(new_socket_fd, (char *)&size_network_order + size_recv_total, size_left - size_recv_total, 0);
        if (received < 0)
        {
            perror("recv size");
            closesocket(new_socket_fd);
            WSACleanup();
            return 1;
        }
        else if (received == 0)
        {
            printf("Connection closed by the client.\n");
            closesocket(new_socket_fd);
            WSACleanup();
            return 1;
        }
        size_recv_total += received;
    }
    int size = ntohl(size_network_order); // Convert to host byte order
    cout << "Data Size : " << size << endl;

    // Receive method
    clear_buffer(buffer, BUFFER_SIZE);
    ssize_t method_recv = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
    if (method_recv <= 0)
    {
        perror("Error receiving method");
        closesocket(new_socket_fd);
        WSACleanup();
        return 1;
    }
    buffer[method_recv] = '\0'; // Null-terminate the received data
    string method(buffer);
    cout << "Method: " << method << endl;

    // Receive redundant error checking data
    clear_buffer(buffer, BUFFER_SIZE);
    ssize_t data_recv = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
    if (data_recv <= 0)
    {
        perror("Error receiving redundant error checking data");
        closesocket(new_socket_fd);
        WSACleanup();
        return 1;
    }
    buffer[data_recv] = '\0'; // Null-terminate the received data
    string data(buffer);
    cout << "Redundant Error Checking Data: " << data << endl;

    vector<string> packets;
    while (true)
    {
        clear_buffer(buffer, BUFFER_SIZE);
        ssize_t packet_recv = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (packet_recv <= 0)
        {
            perror("Error receiving packet");
            break; // Exit on error
        }
        buffer[packet_recv] = '\0'; // Null-terminate the received data
        string packet(buffer);
        if (packet == "end")
        {
            break;
        }
        cout << "Received packet: " << packet << endl;
        packets.push_back(packet);
    }

    // Validate and send response
    string response;
    if (method == "CRC")
    {
        cout << "Divisor: " << data << endl;
        if (validate_crc(packets, data))
       
        {
            response = "Valid - No Error Found By CRC";
        }
        else
        {
            response = "Invalid - Error Found By CRC";
        }
    }
    else if (method == "Checksum")
    {
        if (validate_checksum(packets, data))
        {
            response = "Valid - No Error Found By Checksum";
        }
        else
        {
            response = "Invalid - Error Found By Checksum";
        }
    }
    else
    {
        cout << "Invalid method\n";
        response = "Invalid Method";
    }

    // Send response to the client
    send(new_socket_fd, response.c_str(), response.size(), 0);

    // Clean up and close sockets
    closesocket(new_socket_fd);
    closesocket(socket_fd);
    WSACleanup();
    return 0;
}
