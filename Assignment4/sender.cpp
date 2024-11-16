#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <thread>
#include <sstream>

#pragma comment(lib, "ws2_32.lib")  // Link Winsock2 library

using namespace std;

// Walsh codes generator
vector<vector<int>> generateWalshCodes(int n) {
    vector<vector<int>> walsh(n, vector<int>(n, 1));
    for (int size = 2; size <= n; size *= 2) {
        for (int i = 0; i < size / 2; i++) {
            for (int j = 0; j < size / 2; j++) {
                walsh[i + size / 2][j] = walsh[i][j];
                walsh[i][j + size / 2] = walsh[i][j];
                walsh[i + size / 2][j + size / 2] = -walsh[i][j];
            }
        }
    }
    return walsh;
}

// Sender class
class Sender {
private:
    int stationId;
    vector<int> dataBits;
    vector<int> codeWord;
    SOCKET socket;

public:
    Sender(int id, const vector<int>& data, const vector<int>& code) : stationId(id), dataBits(data), codeWord(code) {}

    void connectToReceiver(const string& serverIP, int port) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "Sender " << stationId << " failed to initialize Winsock.\n";
            exit(EXIT_FAILURE);
        }

        if ((socket = ::socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            cerr << "Sender " << stationId << " failed to create socket.\n";
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

        if (connect(socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Sender " << stationId << " failed to connect to receiver.\n";
            closesocket(socket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }
        cout << "Sender " << stationId << " connected to receiver.\n";
    }

    void transmitData() {
        vector<int> encodedData(dataBits.size() * codeWord.size());
        int idx = 0;

        // Encode data using Walsh code
        for (int bit : dataBits) {
            for (int code : codeWord) {
                encodedData[idx++] = bit * code;
            }
        }

        // Prepare transmission string
        stringstream ss;
        ss << stationId << " ";
        for (int val : encodedData) {
            ss << val << " ";
        }
        string transmission = ss.str();

        // Send encoded data to receiver
        send(socket, transmission.c_str(), transmission.size(), 0);
        cout << "Sender " << stationId << " transmitted encoded data.\n";
    }

    ~Sender() {
        closesocket(socket);
        WSACleanup();
    }
};

int main() {
    const string SERVER_IP = "127.0.0.1";
    const int SERVER_PORT = 5000;

    // Number of stations and data bits
    const int NUM_STATIONS = 4;
    vector<vector<int>> walshCodes = generateWalshCodes(NUM_STATIONS);

    // Sample data for each station
    vector<vector<int>> data = {
        {1, -1, 1},
        {-1, 1, -1},
        {1, 1, -1},
        {-1, -1, 1}
    };

    vector<thread> senderThreads;

    // Launch senders
    for (int i = 0; i < NUM_STATIONS; i++) {
        senderThreads.emplace_back([i, &data, &walshCodes, &SERVER_IP, SERVER_PORT]() {
            Sender sender(i, data[i], walshCodes[i]);
            sender.connectToReceiver(SERVER_IP, SERVER_PORT);
            sender.transmitData();
        });
    }

    // Wait for all threads to complete
    for (auto& t : senderThreads) {
        t.join();
    }

    return 0;
}
