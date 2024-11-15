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
        int expected_frame = 0;
        while (true)
        {
            clear_buffer(buffer, BUFFER_SIZE);
            int recv_len = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
            if (recv_len == SOCKET_ERROR)
            {
                error("Error receiving data");
            }
            buffer[recv_len] = '\0';
            string received_frame_str(buffer);
            cout << "Received Frame: " << received_frame_str << endl;
            if (received_frame_str == "exit")
            {
                cout << "Exiting..." << endl;
                break;
            }
            Frame received_frame = Frame::parseFrame(received_frame_str);
            cout << "------------------------------------------------------------------";
            // Frame::show(received_frame);
            if (received_frame.validateCRC())
            {
                if (received_frame.frame_seq == expected_frame)
                {
                    cout << "Frame " << received_frame.frame_seq << " received" << endl;
                    Frame::processFrame(received_frame);

                    string ack = to_string(received_frame.frame_seq);
                    send(socket_fd, ack.c_str(), ack.size(), 0);
                    cout << "Acknowledgment for frame " << received_frame.frame_seq << " sent" << endl;

                    expected_frame++;
                }
                else
                {
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

class GoBackNReceiver
{
private:
    vector<Frame> packets;
    int total_frame;
    int window_size;
    int expected_frame_r = 0;

public:
    GoBackNReceiver(int t, int ws)
    {
        total_frame = t;
        window_size = ws;
    }

    void receiveGoBackN(SOCKET socket_fd)
    {
        char buffer[BUFFER_SIZE];

        // while (true)
        // {
        //     clear_buffer(buffer, BUFFER_SIZE);
        //     int recv_len = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
        //     if (recv_len == SOCKET_ERROR)
        //     {
        //         error("Error receiving data");
        //     }
        //     buffer[recv_len] = '\0';
        //     string received_frame_str(buffer);
        //     cout << "Received Frame: " << received_frame_str << endl;
        //     if (received_frame_str == "exit")
        //     {
        //         cout << "Exiting..." << endl;
        //         break;
        //     }
        //     Frame received_frame = Frame::parseFrame(received_frame_str);
        //     if (received_frame.validateCRC())
        //     {
        //         if (received_frame.frame_seq == expected_frame_r)
        //         {
        //             cout << "Frame " << received_frame.frame_seq << " received" << endl;
        //             packets.push_back(received_frame);
        //             string ack = to_string(received_frame.frame_seq);
        //             send(socket_fd, ack.c_str(), ack.size(), 0);
        //             cout << "Acknowledgment for frame " << received_frame.frame_seq << " sent" << endl;
        //             expected_frame_r = (expected_frame_r + 1) % window_size;
        //             cout <<"---------------------------------------------------------------------"<<endl;

        //         }
        //         else if (received_frame.frame_seq < expected_frame_r)
        //         {
        //             string ack = to_string(received_frame.frame_seq);
        //             send(socket_fd, ack.c_str(), ack.size(), 0);
        //             cout << "Frame out of order" << ack << endl;
        //         }
        //         else
        //         {
        //             string ack = to_string(expected_frame_r - 1);
        //             send(socket_fd, ack.c_str(), ack.size(), 0);
        //             cout << "Frame out of order" << ack << endl;
        //         }
        //     }
        // }
        while (true)
        {
            char buffer[1024] = {0};
            int bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
                break;

            std::string frame(buffer, bytesRead);
            if (frame.empty())
                break;
            string received_frame_str(buffer);
            cout << "Received Frame: " << received_frame_str << endl;
            if (received_frame_str == "exit")
            {
                cout << "Exiting..." << endl;
                break;
            }
            Frame received_frame = Frame::parseFrame(received_frame_str);
            int receivedSeqNo = received_frame.frame_seq;

            // If the received sequence number matches the expected one
            if (receivedSeqNo == expected_frame_r && received_frame.validateCRC())
            {
                cout << "Frame " << received_frame.frame_seq << " received" << endl;
                packets.push_back(received_frame);
                string ack = to_string(received_frame.frame_seq);
                send(socket_fd, ack.c_str(), ack.size(), 0);
                cout << "Acknowledgment for frame " << received_frame.frame_seq << " sent" << endl;
                expected_frame_r = (expected_frame_r + 1) % window_size;
                cout << "---------------------------------------------------------------------" << endl;
                continue;
            }
            else if (receivedSeqNo < expected_frame_r)
            {
                // Handle duplicate frames (already acknowledged)
                std::cout << "Frame out of order. Rejecting Seq No: " << receivedSeqNo << std::endl;
            }
            else if (received_frame.validateCRC())
            {
                std::cout << "Frame Corrupted. Rejecting Seq No: " << receivedSeqNo << std::endl;
            }
            else
            {
                // Frame out of order
                std::cout << "Frame out of order. Rejecting Seq No: " << receivedSeqNo << std::endl;
            }
            string ack = to_string(received_frame.frame_seq);
            int nack = stoi(ack) - 1;
            send(socket_fd, to_string(nack).c_str(), to_string(nack).size(), 0);
        }

        cout << "_____________________________________________________________________________________" << endl;
        cout << "All frames received and acknowledged" << endl;
    }
};
vector<Frame> packets;
class SelectiveRepeatReceiver
{
    int n;
    int total_frame;
    int expected_frame_r = 0;
    
public:
    SelectiveRepeatReceiver(int n,int tf)
    {
        this->n = n;
        this->total_frame = tf;
        for(int i=0;i<total_frame +2;i++)
        {
            packets.push_back(Frame("dummy", "source", "dest", 8, i));
        }
    }
    void receive(SOCKET socket_fd)
    {
        char buffer[BUFFER_SIZE];
        while (true)
        {
            char buffer[1024] = {0};
            int bytesRead = recv(socket_fd, buffer, sizeof(buffer), 0);
            if (bytesRead <= 0)
                break;

            std::string frame(buffer, bytesRead);
            if (frame.empty())
                break;
            string received_frame_str(buffer);
            cout << "Received Frame: " << received_frame_str << endl;
            if (received_frame_str == "exit")
            {
                cout << "Exiting..." << endl;
                break;
            }
            Frame received_frame = Frame::parseFrame(received_frame_str);
            int receivedSeqNo = received_frame.frame_seq;

            // If the received sequence number matches the expected one
            if (received_frame.validateCRC())
            {
                cout << "Frame " << received_frame.frame_seq << " received" << endl;
                packets.push_back(received_frame);
                string ack = to_string(received_frame.frame_seq);
                send(socket_fd, ack.c_str(), ack.size(), 0);
                cout << "Acknowledgment for frame " << received_frame.frame_seq << " sent" << endl;
                packets[received_frame.frame_seq] = received_frame;
                expected_frame_r = (expected_frame_r + 1);
                cout << "---------------------------------------------------------------------" << endl;
                continue;
            }
            else if (receivedSeqNo < expected_frame_r)
            {
                // Handle duplicate frames (already acknowledged)
                std::cout << "Duplicate Frame  " << receivedSeqNo << std::endl;
            }
            else if (received_frame.validateCRC())
            {
                std::cout << "Frame Corrupted. Rejecting Seq No: " << receivedSeqNo << std::endl;
            }
            else
            {
                // Frame out of order
                std::cout << "Frame out of order. Rejecting Seq No: " << receivedSeqNo << std::endl;
            }
            string ack = to_string(received_frame.frame_seq);
            int nack = stoi(ack) - 1;
            send(socket_fd, to_string(nack).c_str(), to_string(nack).size(), 0);
        }

        cout << "_____________________________________________________________________________________" << endl;
        cout << "All frames received and acknowledged" << endl;
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
    buffer[recv_len] = '\0';

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
    buffer[recv_len] = '\0';
    string flow_control(buffer);
    cout << "Flow Control Mechanism: " << buffer << endl;
    if (flow_control == "StopandWait")
    {
        StopAndWait stopAndWait;
        stopAndWait.receiveStopAndWait(new_socket_fd);
    }
    else if (flow_control == "GoBackN")
    {
        clear_buffer(buffer, BUFFER_SIZE);
        recv_len = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (recv_len == SOCKET_ERROR)
        {
            error("Error receiving data");
        }
        buffer[recv_len] = '\0';
        int total_frame = stoi(buffer);
        clear_buffer(buffer, BUFFER_SIZE);
        recv_len = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (recv_len == SOCKET_ERROR)
        {
            error("Error receiving data");
        }
        buffer[recv_len] = '\0';
        int window_size = stoi(buffer);
        cout << "Total Frames: " << total_frame << endl;
        cout << "Window Size of Sender: " << window_size << endl;
        GoBackNReceiver goBackN(total_frame, window_size);
        goBackN.receiveGoBackN(new_socket_fd);
    }
    else if (flow_control == "SelectiveRepeat")
    {
        clear_buffer(buffer, BUFFER_SIZE);
        recv_len = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (recv_len == SOCKET_ERROR)
        {
            error("Error receiving data");
        }
        buffer[recv_len] = '\0';
        int window_size = stoi(buffer);
        cout << "Window Size of Sender: " << window_size << endl;
        clear_buffer(buffer, BUFFER_SIZE);
        recv_len = recv(new_socket_fd, buffer, BUFFER_SIZE - 1, 0);
        if (recv_len == SOCKET_ERROR)
        {
            error("Error receiving data");
        }
        buffer[recv_len] = '\0';
        int tf = stoi(buffer);
        cout << "Total Number of frame : " << tf << endl;
        SelectiveRepeatReceiver selectiveRepeat(window_size,tf);
        selectiveRepeat.receive(new_socket_fd);
    }
    else
    {
        cout << "Invalid Flow Control Mechanism" << endl;
    }
    closesocket(new_socket_fd);
    closesocket(socket_fd);
    WSACleanup();

    return 0;
}
