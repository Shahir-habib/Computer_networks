#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib for Winsock functions
#pragma comment(lib, "Iphlpapi.lib")
#include "framing.h"
using namespace std;

#define BUFFER_SIZE 1024
string MAC_ADDRESS;
void error(const char *msg)
{
    fprintf(stderr, "%s: %d\n", msg, WSAGetLastError());
    exit(1);
}

void clear_buffer(char *buffer, size_t size)
{
    memset(buffer, '\0', size);
}

void get_mac_address(SOCKET client_socket)
{
    // Declare and initialize variables
    PIP_ADAPTER_INFO pAdapterInfo;
    PIP_ADAPTER_INFO pAdapter;
    DWORD dwSize = 0;

    // Get the necessary buffer size
    if (GetAdaptersInfo(NULL, &dwSize) == 111)
    {
        pAdapterInfo = (IP_ADAPTER_INFO *)malloc(dwSize);
        if (pAdapterInfo == NULL)
        {
            printf("Memory allocation failed.\n");
            return;
        }

        // Retrieve the adapter info
        if (GetAdaptersInfo(pAdapterInfo, &dwSize) == 0)
        {
            pAdapter = pAdapterInfo;
            while (pAdapter)
            {
                char mac_address[18];
                snprintf(mac_address, sizeof(mac_address), "%02X-%02X-%02X-%02X-%02X-%02X",
                         pAdapter->Address[0], pAdapter->Address[1],
                         pAdapter->Address[2], pAdapter->Address[3],
                         pAdapter->Address[4], pAdapter->Address[5]);

                // Send the MAC address over the socket connection
                MAC_ADDRESS = string(mac_address);
                send(client_socket, mac_address, strlen(mac_address), 0);

                // Break after sending the first MAC address if multiple are found
                break;
            }
        }
        else
        {
            printf("GetAdaptersInfo failed.\n");
        }

        // Free memory
        free(pAdapterInfo);
    }
    else
    {
        printf("GetAdaptersInfo failed to get buffer size.\n");
    }
}

class StopAndWait
{
    public:
    void receiveStopAndWait(SOCKET socket_fd)
    {
        cout << "Receiving data using Stop and Wait" << endl;
        char buffer[BUFFER_SIZE];
        int expected_frame = 0; // The sequence number of the frame we are expecting

        while (true)
        {
            clear_buffer(buffer, BUFFER_SIZE);
            int recv_len = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
            if (recv_len == SOCKET_ERROR)
            {
                error("Error receiving data");
            }
            buffer[recv_len] = '\0'; // Null-terminate the received data
            string received_frame_str(buffer);
            cout<<"Received Frame: "<<received_frame_str<<endl;
            if (received_frame_str == "exit")
            {
                cout << "Exiting..." << endl;
                break;
            }
            Frame received_frame = Frame::parseFrame(received_frame_str);
            //Frame::show(received_frame);
            if (received_frame.validateCRC())
            {
                if (received_frame.frame_seq == expected_frame)
                {
                    // Process the received frame
                    cout << "Frame " << received_frame.frame_seq << " received" << endl;
                    Frame::processFrame(received_frame); // Your function to process the frame

                    // Send acknowledgment for the received frame
                    string ack = to_string(received_frame.frame_seq);
                    send(socket_fd, ack.c_str(), ack.size(), 0);
                    cout << "Acknowledgment for frame " << received_frame.frame_seq << " sent" << endl;

                    // Update the expected frame sequence number
                    expected_frame++;
                }
                else
                {
                    // If the frame is not the expected one, resend the acknowledgment for the last correctly received frame
                    string ack = to_string(expected_frame - 1); // Resend acknowledgment for the last correct frame
                    send(socket_fd, ack.c_str(), ack.size(), 0);
                    cout << "Resent acknowledgment for frame " << (expected_frame - 1) << endl;
                }
            }
            else
            {
                // If CRC validation fails, resend acknowledgment for the last correct frame
                string ack = to_string(expected_frame - 1); // Resend acknowledgment for the last correct frame
                send(socket_fd, ack.c_str(), ack.size(), 0);
                cout << "CRC validation failed. Resent acknowledgment for frame " << (expected_frame - 1) << endl;
            }
        }
    }
};
int main(int argc, char *argv[])
{
    WSADATA wsaData;
    SOCKET socket_fd, new_socket_fd;
    struct sockaddr_in server_addr, client_addr;
    int port_number;
    char buffer[BUFFER_SIZE];
    int client_len = sizeof(client_addr);

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s port\n", argv[0]);
        return 1;
    }

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
        error("Error opening socket");
    }

    // Set up the server address
    port_number = atoi(argv[1]);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port_number);

    // Bind the socket
    if (bind(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
    {
        error("Error on binding");
    }

    // Listen for incoming connections
    if (listen(socket_fd, 5) == SOCKET_ERROR)
    {
        error("Error on listen");
    }

    // Accept a connection
    new_socket_fd = accept(socket_fd, (struct sockaddr *)&client_addr, &client_len);
    if (new_socket_fd == INVALID_SOCKET)
    {
        error("Error on accept");
    }

    printf("Server: Connection established\n");

    // Receive MAC address from the client
    clear_buffer(buffer, BUFFER_SIZE);
    int recv_len = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
    if (recv_len == SOCKET_ERROR)
    {
        error("Error receiving data");
    }
    buffer[recv_len] = '\0'; // Null-terminate the received data

    cout << "MAC Address of Client: " << buffer << endl;

    // Get and send the server's MAC address
    get_mac_address(new_socket_fd);
    // Receive the flow control mechanism
    clear_buffer(buffer, BUFFER_SIZE);
    recv_len = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
    if (recv_len == SOCKET_ERROR)
    {
        error("Error receiving data");
    }
    buffer[recv_len] = '\0'; // Null-terminate the received data
    string flow_control(buffer);
    cout << "Flow Control Mechanism: " << buffer << endl;
    if (flow_control == "StopandWait")
    {
        StopAndWait stopAndWait;
        stopAndWait.receiveStopAndWait(new_socket_fd);
    }
    else if (flow_control == "GoBackN")
    {
    }
    else if (flow_control == "SelectiveRepeat")
    {
    }
    else
    {
        cout << "Invalid Flow Control Mechanism" << endl;
    }
    // Cleanup
    closesocket(new_socket_fd);
    closesocket(socket_fd);
    WSACleanup();

    return 0;
}
