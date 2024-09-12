#include <bits/stdc++.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include <chrono>
#include <thread>
#pragma comment(lib, "ws2_32.lib") // Link with ws2_32.lib for Winsock functions
#pragma comment(lib, "Iphlpapi.lib")
#include "framing.h"
using namespace std;

#define BUFFER_SIZE 1024
string MAC_ADDRESS;
string server_mac_address;
vector<Frame> packets;
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

                // Send the MAC address over the WebSocket connection
                MAC_ADDRESS = string(mac_address);
                send(client_socket, mac_address, strlen(mac_address), 0);
                break; // Send only the first MAC address if multiple are found
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
private:
    chrono::time_point<chrono::steady_clock> start_time;
    int timeout_interval_ms; 
public:
    StopAndWait() : timeout_interval_ms(1000) // Default timeout 1000 ms
    {
    }
    void Timer()
    {
        start_time = chrono::steady_clock::now();
    }
    void Timeout()
    {
        auto end_time = chrono::steady_clock::now();
        chrono::duration<double, milli> round_trip_time = end_time - start_time;
        timeout_interval_ms = static_cast<int>(round_trip_time.count() * 1.5); // Example: 1.5x of round-trip time
    }
    bool has_timed_out()
    {
        auto current_time = chrono::steady_clock::now();
        chrono::duration<double, milli> elapsed_time = current_time - start_time;
        return elapsed_time.count() > timeout_interval_ms;
    }
    void sendstopandwait(SOCKET socket_fd)
    {
        cout << "Sending data using Stop and Wait" << endl;
        int fn = 0;
        bool cansend = true;
        char buffer[BUFFER_SIZE];
        while (true)
        {
            if (fn == packets.size() - 1)
            {
                cout << "All frames sent" << endl;
                send(socket_fd, "exit", 4, 0);
                break;
            }
            if (cansend)
            {
                packets[fn].addCRC();
                string frame = packets[fn].source_addr + packets[fn].dest_addr + to_string(packets[fn].len) +
                                    to_string(packets[fn].frame_seq) + packets[fn].data + packets[fn].trailer;

                send(socket_fd, frame.c_str(), frame.size(), 0);
                cout << "Frame " << fn << " sent" << endl;

                Timer(); // Start the timer when the frame is sent
                // Wait for acknowledgment or timeout
                bool ack_received = false;
                while (!ack_received)
                {
                    if (has_timed_out())
                    {
                        cout << "Frame " << fn << " timed out. Retransmitting..." << endl;
                        send(socket_fd, frame.c_str(), frame.size(), 0); // Retransmit the frame
                        Timer(); // Restart the timer
                    }
                    else
                    {
                        clear_buffer(buffer, BUFFER_SIZE);
                        int recv_len = recv(socket_fd, buffer, BUFFER_SIZE - 1, 0);
                        if (recv_len == SOCKET_ERROR)
                        {
                            error("Error receiving data");
                        }
                        buffer[recv_len] = '\0'; // Null-terminate the received data
                        string ack = buffer;
                        if (buffer == to_string(fn)) // Assuming f_no is the acknowledgment
                        {
                            cout << "Frame " << fn << " acknowledged" << endl;
                            ack_received = true;
                            fn++;
                            cansend = true;
                        }
                        else
                        {
                            cout << "Invalid acknowledgment received" <<ack<< endl;
                            break;
                        }
                    }
                    // Sleep briefly to avoid busy waiting
                    this_thread::sleep_for(chrono::milliseconds(100));
                }
            }
            else
            {
                // Continue the loop if we can't send the frame yet
                this_thread::sleep_for(chrono::milliseconds(100));
            }
        }
    }
};
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
        fprintf(stderr, "Usage: %s hostname port\n", argv[0]);
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

    // Get and send MAC address
    get_mac_address(socket_fd);

    // Receive response from server
    clear_buffer(rebuffer, BUFFER_SIZE);
    int recv_len = recv(socket_fd, rebuffer, BUFFER_SIZE - 1, 0);
    if (recv_len == SOCKET_ERROR)
    {
        error("Error receiving data");
    }
    rebuffer[recv_len] = '\0'; // Null-terminate the received data

    // Output received MAC address
    for (int i = 0; i < recv_len; i++)
    {
        server_mac_address += rebuffer[i];
    }
    cout << "MAC Address received from server: " << rebuffer << endl;
    
    FILE *file = fopen(argv[3], "rb");
    if (file == NULL)
    {
        error("Error opening file");
    }
    char ch;
    string text = "";
    while ((ch = fgetc(file)) != EOF)
    {
        text += ch;
    }
    fclose(file);
    for (size_t i = 0; i < text.size(); i += 8)
    {
        packets.push_back(Frame(text.substr(i, 8), MAC_ADDRESS, server_mac_address, 8, i / 8));
    }
    cout << "Enter the flow Control Mechanism by which you want to send the data (StopAndWait/GoBackN/SelectiveRepeat): ";
    string flowControl;
    cin >> flowControl;
    send(socket_fd, flowControl.c_str(), flowControl.size(), 0);
    if (flowControl == "StopandWait")
    {
        StopAndWait sw;
        sw.sendstopandwait(socket_fd);
    }
    else if (flowControl == "GoBackN")
    {
    }
    else if (flowControl == "SelectiveRepeat")
    {
    }
    else
    {
        cout << "Invalid Flow Control Mechanism" << endl;
    }
    // Cleanup
    closesocket(socket_fd);
    WSACleanup();
    return 0;
}
