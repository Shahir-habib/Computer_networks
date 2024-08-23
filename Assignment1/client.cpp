#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
using namespace std;
#include "checksum.h"
#include "crc.h"
#include "inject_error.h"
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib for Winsock functions
#pragma comment(lib, "iphlpapi.lib")

#define BUFFER_SIZE 255

void error(const char *msg)
{
    fprintf(stderr, "%s: %d\n", msg, WSAGetLastError());
    exit(1);
}

void clear_buffer(char *buffer, size_t size)
{
    memset(buffer, '\0', size);
}

int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET socket_fd;
    struct sockaddr_in server_addr;
    struct hostent *server;
    int port_number;
    char buffer[BUFFER_SIZE], rebuffer[BUFFER_SIZE];

    if (argc < 4)
    {
        fprintf(stderr, "Usage: %s hostname port file_name\n", argv[0]);
        return 1;
    }

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        printf("WSAStartup failed.\n");
        return 1;
    }

    // Create a socket
    port_number = atoi(argv[2]);
    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == INVALID_SOCKET)
    {
        error("Error opening socket");
    }
    // Resolve the hostname
    server = gethostbyname(argv[1]);
    if (server == NULL)
    {
        fprintf(stderr, "Error, no such host\n");
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    // Set up the server address
    server_addr.sin_family = AF_INET;
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    server_addr.sin_port = htons(port_number);
    clear_buffer(buffer, BUFFER_SIZE);

    // Connect to the server
    if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        error("Error connecting");
    }

    FILE *file = fopen(argv[3], "rb");
    if (file == NULL)
    {
        error("Error opening file");
    }
    char ch;
    string method;
    cout << "Enter the method you want to use (CRC/Checksum): ";
    cin >> method;
    string text = "";
    vector<string> packets;
    while ((ch = fgetc(file)) != EOF)
    {
        text += ch;
    }
    fclose(file);

    // Pad text to be a multiple of 8 bits
    if (text.size() % 8 != 0)
    {
        text += string(8 - text.size() % 8, '0');
    }

    for (size_t i = 0; i < text.size(); i += 8)
    {
        packets.push_back(text.substr(i, 8));
    }
    // Sending size of the incoming data
    int size = text.size();
    int size_network_order = htonl(size); // Convert to network byte order
    ssize_t total_sent = 0;
    ssize_t size_left = sizeof(size_network_order);

    while (total_sent < size_left)
    {
        ssize_t sent = send(socket_fd, (char *)&size_network_order + total_sent, size_left - total_sent, 0);
        if (sent == SOCKET_ERROR)
        {
            error("send size");
        }
        total_sent += sent;
    }
    // sending method
    if (method == "CRC")
    {
        const char *method_str = "CRC";
        send(socket_fd, method_str, strlen(method_str), 0);
    }
    else if (method == "Checksum")
    {
        const char *method_str = "Checksum";
        send(socket_fd, method_str, strlen(method_str), 0);
    }
    else
    {
        cout << "Invalid method\n";
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    // Additional method-specific handling
    if (method == "CRC")
    {
        cout << "Which CRC method do you want to use? (CRC-8, CRC-10, CRC-16, CRC-32): ";
        string crc_method;
        cin >> crc_method;

        string divisor = get_divisor(crc_method);
        send(socket_fd, divisor.c_str(), divisor.size(), 0);

        packets = generate_crc(packets, crc_method);
    }
    else if (method == "Checksum")
    {
        // Generate checksum
        cout << "Genrating checksum\n";
        string checksum = generate_checksum(text, 8);
        cout << "Checksum: " << checksum << endl;
        send(socket_fd, checksum.c_str(), checksum.size(), 0);
    }
    cout << "Want to inject error? (y/n): ";
    char choice;
    cin >> choice;
    if (choice == 'y')
    {
        int random_index = rand() % packets.size();
        ask(packets[random_index]);
    }
    cout << "Sending packets\n";
    for (const string &packet : packets)
    {
        ssize_t packet_total_sent = 0;
        ssize_t packet_size_left = packet.size();
        while (packet_total_sent < packet_size_left)
        {
            ssize_t sent = send(socket_fd, packet.c_str() + packet_total_sent, packet_size_left - packet_total_sent, 0);
            if (sent == SOCKET_ERROR)
            {
                error("send packet");
            }
            packet_total_sent += sent;
        }
        cout << packet << endl;
    }

    send(socket_fd, "end", 3, 0);
    cout << "Packets sent\n";

    // Receive a response from the server
    clear_buffer(rebuffer, BUFFER_SIZE);

    ssize_t n = recv(socket_fd, rebuffer, sizeof(rebuffer) - 1, 0);
    if (n == SOCKET_ERROR)
    {
        error("recv");
    }
    else if (n == 0)
    {
        cout << "Server closed connection\n";
        closesocket(socket_fd);
        WSACleanup();
        return 1;
    }

    rebuffer[n] = '\0'; // Null-terminate the received data
    cout << "Received response: " << rebuffer << endl;

    closesocket(socket_fd);
    WSACleanup();
    return 0;
}