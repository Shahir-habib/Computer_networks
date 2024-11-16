#include <iostream>
#include <vector>
#include <string>
#include <winsock2.h>
#include <sstream>
#include <thread>

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

// Receiver class
class Receiver {
private:
    int numStations;
    vector<vector<int>> walshCodes;
    vector<vector<int>> receivedData;
    SOCKET serverSocket;

    void decodeData() {
        for (size_t station = 0; station < receivedData.size(); station++) {
            vector<int>& encodedData = receivedData[station];
            vector<int> decodedData;

            for (size_t i = 0; i < encodedData.size(); i += walshCodes[0].size()) {
                int sum = 0;
                for (size_t j = 0; j < walshCodes[station].size(); j++) {
                    sum += encodedData[i + j] * walshCodes[station][j];
                }
                decodedData.push_back(sum / walshCodes[station].size());
            }

            cout << "Decoded data for station " << station << ": ";
            for (int bit : decodedData) {
                cout << (bit > 0 ? "1 " : "-1 ");
            }
            cout << endl;
        }
    }

    void handleClient(SOCKET clientSocket) {
        char buffer[1024];
        recv(clientSocket, buffer, sizeof(buffer), 0);

        stringstream ss(buffer);
        int stationId;
        ss >> stationId;

        vector<int> encodedData;
        int val;
        while (ss >> val) {
            encodedData.push_back(val);
        }

        // Store received data
        if (stationId >= 0 && stationId < numStations) {
            receivedData[stationId] = encodedData;
        }

        closesocket(clientSocket);
    }

public:
    Receiver(int stations) : numStations(stations), receivedData(stations) {
        walshCodes = generateWalshCodes(numStations);
    }

    void start(int port) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cerr << "Receiver failed to initialize Winsock.\n";
            exit(EXIT_FAILURE);
        }

        if ((serverSocket = ::socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
            cerr << "Receiver failed to create socket.\n";
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;

        if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            cerr << "Receiver failed to bind socket.\n";
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        if (listen(serverSocket, numStations) == SOCKET_ERROR) {
            cerr << "Receiver failed to listen.\n";
            closesocket(serverSocket);
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        cout << "Receiver listening on port " << port << "...\n";

        vector<thread> clientThreads;

        for (int i = 0; i < numStations; i++) {
            SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
            if (clientSocket != INVALID_SOCKET) {
                clientThreads.emplace_back(&Receiver::handleClient, this, clientSocket);
            }
        }

        // Wait for all client threads
        for (auto& t : clientThreads) {
            t.join();
        }

        decodeData();

        closesocket(serverSocket);
        WSACleanup();
    }
};

int main() {
    const int NUM_STATIONS = 4;
    const int PORT = 5000;

    Receiver receiver(NUM_STATIONS);
    receiver.start(PORT);

    return 0;
}
